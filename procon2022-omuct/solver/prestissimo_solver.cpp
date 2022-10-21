
#include "prestissimo_solver.h"
#include "prestissimo_small_solver.h"
#include "prestissimo_problem_solver.h"
#include "solver_interface_queue.h"

#include "thread_pool.h"


namespace Procon33 {

	namespace Prestissimo {

		class SolverImpl : public Solver {

			std::shared_ptr<const PreInfo> m_preInfo;
			std::map<String, std::shared_ptr<ProblemSolver>> m_problemSolvers;
			std::map<String, std::shared_ptr<const ProblemInfo>> m_problemInfo;
			std::shared_ptr<const PrecalcUnit> m_precalc;
			std::shared_ptr<SolverInternal::SolverInterfaceQueue> m_queue;
			std::shared_ptr<SolverState> m_state;
			Array<std::shared_ptr<const ProblemAdditionalData>> m_allAdditionalData;

			int32 m_threadCount = 6;
			std::shared_ptr<Array<std::shared_ptr<SolverSmall>>> m_threadWorkMemory;

			std::thread m_controlThread;

			String m_solvingProblemId;
			std::shared_ptr<ProblemSolver> m_currentProblemSolver;
			std::mutex m_solvingProblemMutex;

			bool didPrecalc() const {
				return (bool)m_preInfo;
			}

			bool hasProblemInfo() const {
				return (bool)m_currentProblemSolver;
			}

			std::shared_ptr<ProblemSolver> getCurrentProblemSolver() {
				auto lock = std::lock_guard(m_solvingProblemMutex);
				return m_currentProblemSolver;
			}
			void setCurrentProblemSolver(String id) {
				{
					auto lock = std::lock_guard(m_solvingProblemMutex);
					m_solvingProblemId = id;
				}
				m_currentProblemSolver = m_problemSolvers.at(id);
				updateStateRaw();
			}

			void updateStateRaw() {
				auto lock = std::lock_guard(m_solvingProblemMutex);
				SolverStateRaw res;
				res.precalcUnit = m_precalc;
				res.additionalData = m_allAdditionalData;
				res.problemInfo = m_problemInfo;
				res.threadCount = m_threadCount;
				res.solvingProblemId = m_solvingProblemId;
				m_state->setStateRaw(std::move(res));
			}

		public:
			void setThreadCountInternal(int32 threadCount) {
				m_threadCount = threadCount;
				m_threadWorkMemory->resize(threadCount);
				for (auto& p : *m_threadWorkMemory) if (!(bool)p) p = std::make_shared<SolverSmall>();
			}

			void precalc(std::shared_ptr<const SolverInternal::RequestPreCalc> req) {
				m_state->setProgressBarState(PROGRESS_ID_PRECALC, 0.0);

				// 対象の wav ファイルを列挙
				Array<String> filePaths = GetPrecalcFileList(req->targetDir, req->targetTags);

				auto preInfo = std::make_shared<PreInfo>();
				preInfo->readers = filePaths.map(
					[](StringView f) -> Prestissimo::Reader {
						Prestissimo::Reader res;
						res.fileName = f;
						res.wave = WaveInt32::FromWavFile(f);
						return res;
					}
				);
				m_preInfo = preInfo;

				int sourceCount = (int)m_preInfo->readers.size();

				auto precalcBuf = std::make_shared<PrecalcUnit>();
				precalcBuf->readers.resize(sourceCount);

				const int32 COST_COPY = 1;
				const int32 COST_TRANSFORM = 4;

				std::atomic<int32> progress = 0;
				const int32 progressDivider = sourceCount * (COST_COPY + COST_TRANSFORM) * 4;

				auto addProgress = [&](int t) {
					progress += t;
					m_state->setProgressBarState(PROGRESS_ID_PRECALC, double(progress.load()) / double(progressDivider));
				};

				auto threadPool = Procon33::ThreadPool::Construct(m_threadCount);

				for (int k = 0; k < sourceCount; k++) {
					threadPool->pushTask([&, k](uint32)->void {
						auto& wave = m_preInfo->readers[k].wave;
						int len = std::max(0, (int)wave.length());
						precalcBuf->readers[k].waveDiff.resize(len);
						for (int t = 0; t < len - 1; t++) precalcBuf->readers[k].waveDiff[t + 1] = wave[t + 1] - wave[t];
						precalcBuf->readers[k].waveDiff[len - 1] = -wave[len - 1];
						addProgress(COST_COPY);
					});
				}

				for (int k = 0; k < sourceCount; k++) {
					threadPool->pushTask([&, k](uint32)->void {
						precalcBuf->readers[k].wave = m_preInfo->readers[k].wave;
						addProgress(COST_COPY);
					});
				}

				threadPool->sync();

				for (int k = 0; k < sourceCount; k++) {
					threadPool->pushTask([&, k](uint32)->void {
						auto revsource = precalcBuf->readers[k].wave.getArray().reversed().map([](int32 x)->int64 { return (int64)x; });
						precalcBuf->readers[k].fpsSourceReversed.write(revsource);
						addProgress(COST_TRANSFORM);
					});
					threadPool->pushTask([&, k](uint32)->void {
						auto revsource = precalcBuf->readers[k].waveDiff.reversed().map([](int32 x)->int64 { return (int64)x; });
						precalcBuf->readers[k].fpsSourceDiffReversed.write(revsource);
						addProgress(COST_TRANSFORM);
					});
				}

#ifdef ENABLE_CUDA_ALGORITHM
				threadPool->pushTask([&](uint32)->void {
					for (int k = 0; k < sourceCount; k++) {
						auto revsource = precalcBuf->readers[k].wave.getArray().reversed().map([](int32 x)->int64 { return (int64)x; });
						precalcBuf->readers[k].fpsSourceReversedCuda.write(revsource);
						addProgress(COST_TRANSFORM);
						auto revsource2 = precalcBuf->readers[k].waveDiff.reversed().map([](int32 x)->int64 { return (int64)x; });
						precalcBuf->readers[k].fpsSourceDiffReversedCuda.write(revsource2);
						addProgress(COST_TRANSFORM);
					}
				});
#endif

				precalcBuf->preInfo = preInfo;

				threadPool->sync();
				m_precalc = precalcBuf;

				for (auto a : m_problemSolvers) a.second->changePrecalc(m_precalc);
				updateStateRaw();
			}

			void resetProblem() {
				if (m_state->isRunning()) {
					throw Error(U"solver のエラー。Prestissimo::Solver 実行中に resetProblem が呼ばれました。");
				}
				m_problemInfo.clear();
				m_problemSolvers.clear();
				updateStateRaw();
			}

			void giveProblemInfo(std::shared_ptr<const ProblemInfo> problemInfo) {
				m_problemInfo[problemInfo->problemId] = problemInfo;
				auto newProblemSolver = GetProblemSolverInstance(problemInfo, m_state, m_threadWorkMemory);
				m_problemSolvers[problemInfo->problemId] = newProblemSolver;
				if ((bool)m_precalc) newProblemSolver->changePrecalc(m_precalc);
			}

			void giveAdditionalData(std::shared_ptr<const ProblemAdditionalData> additionalData) {
				if (!(bool)additionalData) return;
				String problemId = additionalData->problemId;
				if (m_problemInfo.count(problemId) == 0) return;

				m_problemSolvers[problemId]->giveAdditionalData(additionalData);
				m_allAdditionalData.push_back(additionalData);
			}

			bool m_stopRequested = false;
			void step() {
				String problemId = U"";
				{
					std::lock_guard lock(m_solvingProblemMutex);
					problemId = m_solvingProblemId;
				}
				m_problemSolvers[problemId]->step();
			}

			void run() {
				bool running = false;

				while (!m_state->getTerminateRequest()) {
					updateStateRaw();

					auto reqWrap = m_queue->pop();
					if (!m_state->getInturruption() && reqWrap.isNull()) {
						if (running) {
							step();
						}
						else {
							System::Sleep(100.0ms);
						}
						continue;
					}

					if (reqWrap.isTheType<std::shared_ptr<SolverInternal::RequestPreCalc>>()) {
						auto req = std::get<std::shared_ptr<SolverInternal::RequestPreCalc>>(reqWrap.get());
						m_state->setProgressBarState(PROGRESS_ID_PRECALC, 0.0);
						precalc(req);
						m_state->deleteProgressBarState(PROGRESS_ID_PRECALC);
						continue;
					}

					if (reqWrap.isTheType<std::shared_ptr<SolverInternal::RequestAddProblem>>()) {
						auto req = std::get<std::shared_ptr<SolverInternal::RequestAddProblem>>(reqWrap.get());
						giveProblemInfo(std::make_shared<ProblemInfo>(std::move(req->problemInfo)));
						continue;
					}

					if (reqWrap.isTheType<std::shared_ptr<SolverInternal::RequestAddChunk>>()) {
						auto req = std::get<std::shared_ptr<SolverInternal::RequestAddChunk>>(reqWrap.get());
						auto arg = std::make_shared<ProblemAdditionalData>();
						arg->partIndex = req->chunkIndex;
						arg->wave = std::move(req->wave);
						arg->problemId = req->problemId;
						giveAdditionalData(arg);
						continue;
					}

					if (reqWrap.isTheType<std::shared_ptr<SolverInternal::RequestSetProblem>>()) {
						auto req = std::get<std::shared_ptr<SolverInternal::RequestSetProblem>>(reqWrap.get());
						setCurrentProblemSolver(req->problemId);
						continue;
					}

					if (reqWrap.isTheType<std::shared_ptr<SolverInternal::RequestChangeThreadCount>>()) {
						auto req = std::get<std::shared_ptr<SolverInternal::RequestChangeThreadCount>>(reqWrap.get());
						setThreadCountInternal(req->threadCount);
						continue;
					}

					if (reqWrap.isTheType<std::shared_ptr<SolverInternal::RequestRun>>()) {
						auto req = std::get<std::shared_ptr<SolverInternal::RequestRun>>(reqWrap.get());
						running = req->run;
						continue;
					}
				}

			}


			// --------------  SolverInterface  --------------------   以下、メインスレッドから呼び出される

			Font m_guiFont = Font(20);

			SolverImpl()
				: m_state(std::make_shared<SolverState>())
				, m_queue(std::make_shared<SolverInternal::SolverInterfaceQueue>())
				, m_threadWorkMemory(std::make_shared<Array<std::shared_ptr<SolverSmall>>>())
			{
				setThreadCountInternal(6);
				m_controlThread = std::thread([this]() { run(); });
			}

			~SolverImpl() {
				auto req = std::make_shared<SolverInternal::RequestRun>();
				req->run = false;
				m_queue->request(req);
				m_state->requestTerminate();
				inturrupt();
				m_controlThread.join();
			}

			void inturrupt() {
				m_state->setInturruption();
			}

			// targetTags は "J" または "E" を含む。
			// targetTags で指定された言語の読みデータをディレクトリ targetDir から検索し、そこから前計算を起動する。
			// 進行度 Solver_Precalc を起動。
			bool precalc(StringView targetDir, Array<String> targetTags) {
				try {
					auto req1 = std::make_shared<SolverInternal::RequestRun>();
					req1->run = false;
					m_queue->request(req1);
					auto req2 = std::make_shared<SolverInternal::RequestPreCalc>();
					req2->targetDir = targetDir;
					req2->targetTags = std::move(targetTags);
					m_queue->request(req2);
					inturrupt();
				}
				catch (const Error& e) {
					m_lastError = e.what();
					return false;
				}
				return true;
			}

			// 現在の試合に問題の情報を追加し、解く問題を変更する。
			// 同じ ID の情報で繰り返し呼ぶと、上書きする。
			bool setProblem(const ProblemInfo& problemInfo) {
				try {
					auto reqAdd = std::make_shared<SolverInternal::RequestAddProblem>();
					reqAdd->problemInfo = problemInfo;
					m_queue->request(reqAdd);
					auto reqSet = std::make_shared<SolverInternal::RequestSetProblem>();
					reqSet->problemId = problemInfo.problemId;
					m_queue->request(reqSet);
					inturrupt();
				}
				catch (const Error& e) {
					m_lastError = e.what();
					return false;
				}
				return true;
			}
			bool setProblem(String problemId) {
				try {
					auto reqSet = std::make_shared<SolverInternal::RequestSetProblem>();
					reqSet->problemId = problemId;
					m_queue->request(reqSet);
					inturrupt();
				}
				catch (const Error& e) {
					m_lastError = e.what();
					return false;
				}
				return true;
			}

			// 与えられる波形を追加。
			// 波形の位置関係を表す番号がファイル名に振られているので chunkIndex に与える。わからない場合は -1 でもよい。
			bool addChunk(StringView problemId, int32 chunkIndex, WaveInt32 wave) {
				try {
					auto req = std::make_shared<SolverInternal::RequestAddChunk>();
					req->chunkIndex = chunkIndex;
					req->problemId = problemId;
					req->wave = std::move(wave);
					m_queue->request(req);
					inturrupt();
				}
				catch (const Error& e) {
					m_lastError = e.what();
					return false;
				}
				return true;
			}

			// true を与えるとソルバーを起動する。
			// false を与えるとソルバーを止める。
			// 進行度 Solver Stop を起動。
			bool runOrStop(bool run) {
				try {
					auto req = std::make_shared<SolverInternal::RequestRun>();
					req->run = run;
					m_queue->request(req);
					inturrupt();
				}
				catch (const Error& e) {
					m_lastError = e.what();
					return false;
				}
				return true;
			}

			// メインの計算に使用するスレッドの個数を設定する。
			// （たぶん）メモリが律速なので threadCount を上げても高速化しないかも
			bool setThreadCount(int32 threadCount) {
				try {
					auto req = std::make_shared<SolverInternal::RequestChangeThreadCount>();
					req->threadCount = threadCount;
					m_queue->request(req);
					inturrupt();
				}
				catch (const Error& e) {
					m_lastError = e.what();
					return false;
				}
				return true;
			}

			// globalRect に収まるように、情報を描画。
			void draw(RectF globalRect) {
				auto stateRaw = m_state->getStateRaw();
				if (stateRaw->solvingProblemId != U"") m_problemSolvers.at(stateRaw->solvingProblemId)->draw(globalRect);
			}

			bool isRunning() { return m_queue->getState().running; }

			// 非同期処理の進行度を 0.0 から 1.0 で表す。
			// "Solver_" から始まる ID と、進行度の 値を組にして返す。
			Array<std::pair<String, double>> getProgress() {
				return m_state->getProgressBarsState();
			}

			AnswerRequestData getAnswer() {
				AnswerRequestData res;
				res.answers = m_state->getAnswer().readPos.map(
					[&](Prestissimo::ReadDataPosition k) -> String {
						String filename = FileSystem::FileName(m_preInfo->readers[k.readId].fileName);
						return filename.substr(1, filename.size() - 5); // Jxx.wav -> xx
					}
				);
				{
					std::lock_guard lock(m_solvingProblemMutex);
					res.problemId = m_solvingProblemId;
				}
				return res;
			}

			std::shared_ptr<SolverFeedback> getFeedback() {
				auto res = std::make_shared<SolverFeedback>();
				for (auto problemSolver : m_problemSolvers) {
					SolverAnswerForProblem ans;
					problemSolver.second->getAnswer();
					res->answer.insert({ problemSolver.first, ans });
				}
				res->lastRequestIndex = 0;
				auto progress = m_state->getProgressBarsState();
				res->progressBarsState = std::map<String, double>(progress.begin(), progress.end());
				for (auto& a : m_precalc->readers) {
					SolverFeedback::ReadDataInfo elem;
					elem.length = (int32)a.wave.length();
					res->readDataInfo.push_back(elem);
				}
				return res;
			}

		};


		std::shared_ptr<Solver> GetSolverInstance() {
			return std::shared_ptr<SolverImpl>(new SolverImpl());
		}

	} // namespace Prestissimo

} // namespace Procon33
