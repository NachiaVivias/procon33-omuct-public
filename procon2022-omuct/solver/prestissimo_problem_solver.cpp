
#include "solver_interface.h"
#include "prestissimo_small_solver.h"
#include "prestissimo_solver.h"

#include "thread_pool.h"
#include "prestissimo_problem_solver.h"


namespace Procon33 {

	namespace Prestissimo {

		class ProblemSolverImpl : public ProblemSolver {

			std::shared_ptr<const PrecalcUnit> m_precalc;
			const std::shared_ptr<const ProblemInfo> m_problemInfo; // 問題情報を持っていることは、コンストラクタ - デストラクタ間で保証
			Array<std::shared_ptr<const ProblemAdditionalData>> m_additionalData;
			std::shared_ptr<SolverState> m_state;

			int32 m_threadCount = 6;
			std::shared_ptr<Array<std::shared_ptr<SolverSmall>>> m_threadWorkMemory;

			int32 m_additionalDataCount;

			Array<ChunkCluster> m_clusters;
			Procon33::WaveInt32 m_targetWave;
			Prestissimo::SolverAnswerForProblem m_currentAnswer;
			Procon33::WaveInt32 m_currentWave;

			std::thread m_controlThread;

			bool hasPrecalc() const {
				return (bool)m_precalc;
			}

			bool hasProblemInfo() const {
				return (bool)m_problemInfo;
			}

			void resetAnswer() {
				m_currentAnswer = Prestissimo::SolverAnswerForProblem();
				m_currentAnswer.chunkPos.assign(m_problemInfo->chunksNum, -1);
				for (auto& chunk : m_additionalData) if ((bool)chunk) m_currentAnswer.chunkPos[chunk->partIndex] = 0;
				m_currentWave = WaveInt32(m_currentWave.length());
				m_currentAnswer.lossFunctionValue = getDifferenceFromTarget();
				m_currentAnswer.clusters = m_clusters;
				m_state->setAnswer(m_currentAnswer);
			}

			void changePrecalc(std::shared_ptr<const PrecalcUnit> precalcUnit) {
				resetAnswer();
				m_precalc = precalcUnit;
			}

			int64 costFunction(const Procon33::WaveInt32& x1, const Procon33::WaveInt32& x2) {
				if (x1.length() != x2.length()) {
					throw Error(U"error in `ScreenSolver::costFunction` : x1.size() != x2.size()");
				}
				int n = (int)x1.length();
				int64 res1 = 0, res2 = 0;
				//auto threadPool = Procon33::ThreadPool::Construct(m_threadCount);
				//threadPool->pushTask([&](uint32) {
					for (int i = 0; i < n; i++) {
						int64 a = Procon33::WaveInt32::clampIntoInt16(x1[i]);
						int64 b = Procon33::WaveInt32::clampIntoInt16(x2[i]);
						res1 += (a - b) * (a - b);
					}
				//});
				//threadPool->pushTask([&](uint32) {
					for (int i = 0; i + 1 < n; i++) {
						int64 a0 = Procon33::WaveInt32::clampIntoInt16(x1[i]);
						int64 a1 = Procon33::WaveInt32::clampIntoInt16(x1[i + 1]);
						int64 b0 = Procon33::WaveInt32::clampIntoInt16(x2[i]);
						int64 b1 = Procon33::WaveInt32::clampIntoInt16(x2[i + 1]);
						int64 a = a1 - a0;
						int64 b = b1 - b0;
						res2 += (a - b) * (a - b) * DIFF_COST_MULTIPLIER;
					}
				//});
				//threadPool->sync();
				return res1 + res2;
			}

			long long getDifferenceFromTarget() {
				return costFunction(m_currentWave, m_targetWave);
			}

			Array<Array<DiffScore>> convolutionCostAll() {

				int sourceCount = (int)m_precalc->readers.size();

				//	スコア記録用
				Array<Array<DiffScore>> ans(sourceCount);
				auto part = Range(0, sourceCount - 1).asArray();

				// 前計算チェック
				if (!hasPrecalc()) throw Error(U"ProblemSolver::convolutionCostAll でエラー : !hasPrecalc()");
				if (!hasProblemInfo()) throw Error(U"ProblemSolver::convolutionCostAll でエラー : !hasProblemInfo()");

				std::shared_ptr<Procon33::ConvolutionInerfaceSimd> fpsDiff = std::make_shared<Procon33::ConvolutionInerfaceSimd>();
				std::shared_ptr<Procon33::ConvolutionInerfaceSimd> fpsDiffDiff = std::make_shared<Procon33::ConvolutionInerfaceSimd>();

#ifdef ENABLE_CUDA_ALGORITHM
				std::shared_ptr<FpsCuda::FpsInNTT> fpsDiffCuda = std::make_shared<FpsCuda::FpsInNTT>();
				std::shared_ptr<FpsCuda::FpsInNTT> fpsDiffDiffCuda = std::make_shared<FpsCuda::FpsInNTT>();
#endif

				int32 additionalDataCount = m_problemInfo->chunksNum;
				const int targetLength = (int)m_targetWave.length();

				auto threadPool = Procon33::ThreadPool::Construct(m_threadCount);

				{
					// 目標波形と現時点の答えの波形の差分
					Array<int64> diff(targetLength);
					for (int t = 0; t < targetLength; t++) {
						diff[t] = m_targetWave[t] - Procon33::WaveInt32::clampIntoInt16(m_currentWave[t]);
					}
					Array<int64> diffDiff(std::max(0, targetLength - 1));
					for (int t = 0; t < targetLength - 1; t++) {
						diffDiff[t] = diff[t + 1] - diff[t];
					}

					// 波形の差分の畳み込み準備
					threadPool->pushTask([&](uint32) { fpsDiff->write(diff); });
					threadPool->pushTask([&](uint32) { fpsDiffDiff->write(diffDiff); });
#ifdef ENABLE_CUDA_ALGORITHM
					threadPool->pushTask([&](uint32) {
						fpsDiffCuda->write(diff);
						fpsDiffDiffCuda->write(diffDiff);
					});
					CudaUtility::SyncOrThrow();
#endif
					threadPool->sync();
				}

				for (int i = 0; i < additionalDataCount; i++) {
					if ((bool)m_additionalData[i]) {
						for (int k : part) {
							threadPool->pushTask(
								[&, k](uint32 thId)->void {
#ifdef ENABLE_CUDA_ALGORITHM
									if (thId == 0) {
										m_threadWorkMemory->at(thId)->minCostCuda(
											m_precalc,
											m_problemInfo,
											k,
											ans[k],
											targetLength,
											fpsDiffCuda,
											fpsDiffDiffCuda
										);
									}
									else {
#endif
										m_threadWorkMemory->at(thId)->minCost(
											m_precalc,
											m_problemInfo,
											k,
											ans[k],
											targetLength,
											fpsDiff,
											fpsDiffDiff
										);
#ifdef ENABLE_CUDA_ALGORITHM
									}
#endif
								}
							);
						}
						break;
					}
				}

				threadPool->sync();

				return ans;
			}

		public:

			void precalcForEachAdditionalData() {
				auto c = m_clusters.back(); {
					size_t partLen = 0;
					for (int i = c.leftIndex; i < c.rightIndex; i++) partLen += m_additionalData[i]->wave.length();
					m_targetWave = WaveInt32(partLen);
					size_t innerOff = 0;
					for (int i = c.leftIndex; i < c.rightIndex; i++) {
						m_targetWave.addWave(m_additionalData[i]->wave, (int32)innerOff);
						innerOff += m_additionalData[i]->wave.length();
					}
				}
				m_currentWave = WaveInt32(m_targetWave.length());
				m_currentAnswer.clusters = m_clusters;
				m_currentAnswer.lossFunctionValue = getDifferenceFromTarget();
			}

			void giveAdditionalData(std::shared_ptr<const ProblemAdditionalData> additionalData) {
				resetAnswer();

				int32 chunkId = additionalData->partIndex;

				if (m_additionalData[chunkId]) return;
				m_additionalData[chunkId] = additionalData;
				m_currentAnswer.chunkPos[chunkId] = 0;

				if (m_additionalDataCount == 0) {
					m_targetWave = additionalData->wave;
					m_currentWave = Procon33::WaveInt32(m_targetWave.length());
				}

				m_clusters.clear();
				for (int32 i = 0; i < m_problemInfo->chunksNum; i++) {
					if (!(bool)m_additionalData[i]) continue;
					if (!m_clusters.empty() && m_clusters.back().rightIndex == i) {
						m_clusters.back().rightIndex++;
						m_clusters.back().lengthSample += (int32)(m_additionalData[i]->wave.length());
					}
					else {
						m_clusters.push_back(ChunkCluster{
							.leftIndex = i,
							.rightIndex = i + 1,
							.offset = 0,
							.rangeOffset = 0,
							.lengthSample = (int32)(m_additionalData[i]->wave.length())
						});
					}
				}

				for (auto& a : m_clusters) if (a.leftIndex <= chunkId && chunkId < a.rightIndex) std::swap(a, m_clusters.back());

				precalcForEachAdditionalData();

				m_additionalDataCount++;
			}

			bool m_stopRequested = false;
			void step() {
				if (!hasPrecalc()) throw Error(U"Prestissimo::ProblemSolver::step でエラーが起きました。（ !hasPrecalc() ）");

				m_stopRequested = false;
				auto receiveStopRequest = [&]()->bool { return m_stopRequested = m_stopRequested || m_state->getInturruption(); };
				if (receiveStopRequest()) return;

				auto stopwatch = Stopwatch(StartImmediately::Yes);

				struct Node {
					int64 score = INT64_MIN;
					int idx = 0;
					int slide = 0;
					bool operator<(const Node& r) const { return score < r.score; }
				};

				auto isUsedInAnswer = [&](int k) -> bool {
					for (auto readPos : m_currentAnswer.readPos) if (readPos.readId == k) return true;
					return false;
				};

				auto addTheBestOne = [&](Optional<ReadDataPosition> prev) -> void {

					int64 cand_cost = INT64_MIN;
					int cand_idx = 0;
					int cand_slide = 0;

					auto qAll = convolutionCostAll();

					for (int k = 0; k < (int)m_precalc->readers.size(); k++) if (!isUsedInAnswer(k)) {
						auto& q = qAll[k];
						for (int t = 0; t < (int)q.size(); t++) {
							if (q[t].score > cand_cost) {
								cand_idx = q[t].readId;
								cand_slide = q[t].readSlide;
								cand_cost = q[t].score;
							}
						}
					}

					// append to current answer
					Prestissimo::ReadDataPosition appendAnswer;
					appendAnswer.readId = cand_idx;
					appendAnswer.readOffset = cand_slide;
					m_currentWave.addWave(m_precalc->readers[appendAnswer.readId].wave, appendAnswer.readOffset);
					if (prev.has_value() && m_currentAnswer.lossFunctionValue < (uint64)getDifferenceFromTarget()) {
						m_currentWave.addWave(m_precalc->readers[appendAnswer.readId].wave, appendAnswer.readOffset, - 1);
						appendAnswer = prev.value();
						m_currentWave.addWave(m_precalc->readers[appendAnswer.readId].wave, appendAnswer.readOffset);
					}
					m_currentAnswer.readPos.push_back(appendAnswer);
				};

				auto BufferAnswer = [&]() {
					using Elem = Prestissimo::ReadDataPosition;
					auto buf = m_currentAnswer;
					buf.readPos.sort_by([](Elem l, Elem r) -> bool { return l.readId < r.readId; });
					auto hash = MD5::FromBinary(m_currentWave.getArray().data(), m_currentWave.getArray().size_bytes());
					m_currentAnswer.lossFunctionValue = getDifferenceFromTarget();
					m_state->setAnswer(buf);
				};

				int32 dataNum = m_problemInfo->dataNum;

				if (int32(m_currentAnswer.readPos.size()) < dataNum) {
					while (int32(m_currentAnswer.readPos.size()) < dataNum) {
						addTheBestOne(none);
						BufferAnswer();
						if (receiveStopRequest()) break;
					}
				}
				else {
					for (int i = 0; i < dataNum; i++) {
						std::swap(m_currentAnswer.readPos[i], m_currentAnswer.readPos.back());
						auto prev = m_currentAnswer.readPos.back();
						m_currentWave.addWave(m_precalc->readers[m_currentAnswer.readPos.back().readId].wave, m_currentAnswer.readPos.back().readOffset, -1);
						m_currentAnswer.readPos.pop_back();
						addTheBestOne(prev);
						std::swap(m_currentAnswer.readPos[i], m_currentAnswer.readPos.back());
						BufferAnswer();
						if (receiveStopRequest()) break;
					}
				}

				BufferAnswer();

				m_currentAnswer.lossFunctionValue = getDifferenceFromTarget();

				receiveStopRequest();
			}

			AnswerRequestData getAnswer() {
				AnswerRequestData res;
				res.answers = m_state->getAnswer().readPos.map(
					[&](Prestissimo::ReadDataPosition k) -> String {
						return m_precalc->preInfo->readers[k.readId].fileName.substr(1);
					}
				);
				res.problemId = m_problemInfo->problemId;
				return res;
			}

			bool isStartable() {
				return false;
			}


			// --------------  SolverInterface  --------------------   以下、メインスレッドから呼び出される

			Font m_guiFont = Font(20);

			ProblemSolverImpl(
				std::shared_ptr<const ProblemInfo> problemInfo,
				std::shared_ptr<SolverState> solverState,
				std::shared_ptr<Array<std::shared_ptr<SolverSmall>>> threadWorkMemory
			)
				: m_precalc(nullptr)
				, m_problemInfo(problemInfo)
				, m_state(solverState)
				, m_threadWorkMemory(threadWorkMemory)
				, m_additionalDataCount(0)
			{
				m_additionalData.resize(m_problemInfo->chunksNum);
				m_currentAnswer.chunkPos.assign(m_problemInfo->chunksNum, -1);
				m_state->setAnswer(m_currentAnswer);
				m_threadCount = (int32)threadWorkMemory->size();
			}

			~ProblemSolverImpl() {
			}

			// globalRect に収まるように、情報を描画。
			void draw(RectF globalRect) {

				if (!hasPrecalc() || !hasProblemInfo()) return;

				auto answer = m_state->getAnswer();
				auto stateRaw = m_state->getStateRaw();
				auto preInfo = stateRaw->precalcUnit->preInfo;
				auto additionalData = stateRaw->additionalData.filter([&](auto a) { return a->problemId == m_problemInfo->problemId; });

				int32 maxSampleLen = 10;
				for (auto& cluster : answer.clusters) {
					maxSampleLen = Max(maxSampleLen, cluster.offset + cluster.lengthSample);
				}
				double lp = maxSampleLen * -0.3, rp = maxSampleLen * 1.3;

				double guiScale = Min(1.0, globalRect.h / 11.2 / (globalRect.w / 20.0));

				auto posX = [&](double t) { return (globalRect.x + globalRect.w * Clamp((t - lp) / (rp - lp), 0.0, 1.0)) * guiScale; };
				auto posX2 = [&](double x) { return (globalRect.x + globalRect.w * x) * guiScale; };
				auto posY = [&](double y) { return (globalRect.y + globalRect.w / 20.0 * y) * guiScale; };

				/*  断片データの位置関係  */ {
					double hProblem = 0.8;
					double upOffset = 0.1;
					for (auto& cluster : answer.clusters) {
						int32 leftOffset = cluster.offset;
						int32 width = cluster.lengthSample;
						int32 rightOffset = leftOffset + width;
						auto r = RectF::FromPoints(Vec2(posX(leftOffset), posY(upOffset)), Vec2(posX(rightOffset), posY(upOffset + hProblem)));
						r.draw(Palette::Deepskyblue);
					}
				}

				/* 損失関数の値 */ {
					double fontSize = posY(0.7) - posY(0.0);
					m_guiFont(ToString(answer.lossFunctionValue)).draw(fontSize, posX2(0.05), posY(0.1), Palette::White);
				}

				int32 dataNum = m_problemInfo->dataNum;

				/*  解答案の位置関係  */ {
					double off = 1.0, dy = 0.5;
					double thSep = 1.0;
					double thMain = (posY(dy) - posY(0.0)) * 0.3;
					double fontSize = (posY(dy) - posY(0.0)) * 1.0;
					for (int32 i = 0; i <= dataNum; i++) {
						double y = off + dy * i;
						Line(Vec2(posX(lp), posY(y)), Vec2(posX(rp), posY(y))).draw(thSep);
					}
					for (size_t i = 0; i < answer.readPos.size(); i++) {
						double y = off + dy * (i + 0.5);
						double xl = answer.readPos[i].readOffset;
						double xr = xl + preInfo->readers[answer.readPos[i].readId].wave.length();
						Line(Vec2(posX(xl), posY(y)), Vec2(posX(xr), posY(y))).draw(thMain, Palette::Yellow.withAlpha(192));
					}
					for (size_t i = 0; i < answer.readPos.size(); i++) {
						double y = off + dy * (i + 0.5);
						double x = lp;
						m_guiFont(ToString(answer.readPos[i].readId)).drawAt(fontSize, posX(x) + fontSize * 2, posY(y), Palette::White);
					}
				}
			}

			Array<std::pair<String, double>> getProgress() {
				return m_state->getProgressBarsState();
			}

		};


		std::shared_ptr<ProblemSolver> GetProblemSolverInstance(
			std::shared_ptr<const ProblemInfo> problemInfo,
			std::shared_ptr<SolverState> solverState,
			std::shared_ptr<Array<std::shared_ptr<SolverSmall>>> threadWorkMemory
		) {
			if (!(bool)problemInfo) throw Error(U"Prestissimo::GetProblemSolverInstance でエラーが起きました。（ problemInfo が null です ）");
			if (!(bool)solverState) throw Error(U"Prestissimo::GetProblemSolverInstance でエラーが起きました。（ solverState が null です ）");
			if (!(bool)threadWorkMemory) throw Error(U"Prestissimo::GetProblemSolverInstance でエラーが起きました。（ threadWorkMemory が null です ）");
			return std::shared_ptr<ProblemSolverImpl>(new ProblemSolverImpl(problemInfo, solverState, threadWorkMemory));
		}

	} // namespace Prestissimo

} // namespace Procon33
