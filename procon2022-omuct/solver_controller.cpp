#include "solver_controller.h"
#include "chunk_idx_from_filename.h"


namespace Procon33 {

	SolverController::SolverController(std::shared_ptr<SolverInterface> solver)
		: m_solver(solver)
		, m_serverInterface(AbstractServer::GetRealServerInterface())
	{
	}

	void SolverController::update() {
		m_matchUpdated = false;
	}

	bool SolverController::requestMatchInfo() {
		auto res = m_serverInterface->requestMatchInfo();
		if (res.has_value()) {
			m_matchInfo = res.value();
			m_matchUpdated = true;
		}
		if (!res.has_value()) {
			m_lastError = U"取得に失敗しました。";
			return false;
		}
		m_lastError = U"";
		return true;
	}

	void SolverController::resetProblems() {
		m_problems.clear();
	}

	bool SolverController::requestProblemInfo() {
		if (!m_matchInfo.has_value()) {
			m_lastError = U"試合情報が登録されていません。";
			return false;
		}
		auto res = m_serverInterface->requestProblemInfo();
		if (res.has_value()) {
			bool found = false;
			String problemId = res.value().problemId;
			for (auto& p : m_problems) {
				if (p.info.problemId == problemId) found = true;
			}
			if (!found) {
				ProblemState st;
				st.info = res.value();
				st.chunks = {};
				m_problems.push_back(st);
				if ((bool)m_solver) m_solver->setProblem(st.info);

				auto chunkFilenames = m_serverInterface->requestChunks(1);
				if (chunkFilenames.has_value()) {
					for (String fname : chunkFilenames.value().fileNames) chunkFound(problemId, fname);
				}
			}
			seeChunks(problemId);
		}
		if (!res.has_value()) {
			m_lastError = U"取得に失敗しました。";
			return false;
		}
		m_lastError = U"";
		return true;
	}

	bool SolverController::requestAnotherChunk(Optional<String> problemIdOpt) {
		if (!m_matchInfo.has_value()) {
			m_lastError = U"試合情報が登録されていません。";
			return false;
		}
		String problemId = problemIdOpt.value_or(m_currentProblem);
		if (problemId == U"") {
			m_lastError = U"問題が登録されていません。";
			return false;
		}
		auto problemState = getProblemState(problemId);
		if (!problemState.has_value()) {
			m_lastError = U"問題情報の取得でエラーが発生しました。意図しないエラーです。";
			return false;
		}
		if ((int32)problemState->chunks.size() == problemState->info.chunksNum) {
			m_lastError = U"すべての断片は既に取得しています。";
			return false;
		}
		auto filename = m_serverInterface->requestChunks((int32)(problemState->chunks.size()) + 1);
		if (!filename.has_value()) {
			m_lastError = U"取得に失敗しました。";
			return false;
		}
		for (auto& f : filename->fileNames) chunkFound(problemState->info.problemId, f);
		seeChunks(problemState->info.problemId);
		return true;
	}

	Optional<MatchInfo> SolverController::getMatchInfo() {
		return m_matchInfo;
	}

	Optional<Array<String>> SolverController::getProblemIds() {
		if (!m_matchInfo.has_value()) return none;
		return m_problems.map([](const ProblemState& x) -> String { return x.info.problemId; });
	}

	Optional<SolverController::ProblemState> SolverController::getProblemState(String id) {
		if (!m_matchInfo.has_value()) return none;
		for (auto a : m_problems) {
			if (a.info.problemId == id) return a;
		}
		return none;
	}

	Optional<SolverController::ProblemState> SolverController::getCurrentProblemState() {
		if (m_currentProblem == U"") return none;
		return getProblemState(m_currentProblem);
	}

	bool SolverController::sendTheAnswer() {
		if (!m_matchInfo.has_value()) {
			m_lastError = U"試合情報が登録されていません。";
			return false;
		}
		if (m_currentProblem == U"") {
			m_lastError = U"問題が登録されていません。";
			return false;
		}
		auto ans = m_solver->getAnswer();
		auto res = m_serverInterface->sendAnswer(ans);
		if (!res.has_value()) {
			m_lastError = U"回答の送信に失敗したか、回答が受理されませんでした。";
			return false;
		}
		m_problems[problemIndexFromId(ans.problemId)].answer = res;
		return true;
	}

	bool SolverController::matchUpdated() { return m_matchUpdated; }

	bool SolverController::turnOnSolver() {
		if (!m_matchInfo.has_value()) {
			m_lastError = U"試合情報が設定されていないので、 solver をオンにできません。";
			return false;
		}
		if (!m_solver->runOrStop(true)) {
			m_lastError = m_solver->getLastErrorMessage();
			return false;
		}
		m_lastError = U"";
		return true;
	}

	bool SolverController::turnOffSolver() {
		if (!m_solver->runOrStop(false)) {
			m_lastError = m_solver->getLastErrorMessage();
			return false;
		}
		m_lastError = U"";
		return true;
	}

	bool SolverController::setCurrentProblem(String problemId) {
		if (problemIndexFromId(problemId) < 0) {
			m_lastError = U"設定された問題は存在しません。";
			return false;
		}
		m_currentProblem = problemId;
		m_solver->setProblem(problemId);
		m_lastError = U"";
		return true;
	}

	bool SolverController::precalc(Array<String> languages) {
		if (!m_solver->precalc(U"wav/", languages)) {
			m_lastError = m_solver->getLastErrorMessage();
			return false;
		}
		return true;
	}

	bool SolverController::setServer(std::shared_ptr<AbstractServer> server) {
		if (!(bool)server) {
			m_lastError = U"SolverController::setServer でエラー ： 引数が null でした。";
			return false;
		}
		m_serverInterface = server;
		return true;
	}


	int32 SolverController::problemIndexFromId(String problemId) {
		for (int32 i = 0; i < (int32)m_problems.size(); i++) if (m_problems[i].info.problemId == problemId) return i;
		return -1;
	}

	void SolverController::chunkFound(String problemId, String filename) {
		auto problemIndex = problemIndexFromId(problemId);
		if (problemIndex < 0) return;
		auto& targetProblem = m_problems[problemIndex];
		bool found = false;
		for (auto& p : targetProblem.chunks) if (p.filename == filename) found = true;
		if (!found) {
			targetProblem.chunks.push_back(ProblemState::ChunkState{
				.filename = filename,
				.wav = nullptr,
				.index = GetIndexFromChunkFileName(filename)
			});
		}
	}

	void SolverController::seeChunks(String problemId) {
		auto problemIndex = problemIndexFromId(problemId);
		if (problemIndex < 0) return;
		auto& targetProblem = m_problems[problemIndex];

		for (auto& c : targetProblem.chunks) if (!c.hasWave()) {
			auto wavOpt = m_serverInterface->requestChunkWave(c.filename);
			if (!wavOpt.has_value()) continue;
			auto wav = wavOpt.value();
			c.wav = std::make_shared<Wave>(std::move(wav));
			if ((bool)m_solver) m_solver->addChunk(problemId, c.index, WaveInt32::FromSiv3dWaveLeft(*(c.wav)));
		}
	}

}

