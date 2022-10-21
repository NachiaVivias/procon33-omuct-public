#pragma once
#include "stdafx.h"
#include "HTTP_communication.h"
#include "solver/solver_interface.h"
#include "problem_generator/match_virtual_server.h"

namespace Procon33 {


	class SolverController {
	public:

		struct ProblemState {
			struct ChunkState {
				String filename;
				std::shared_ptr<Wave> wav;
				int32 index;

				bool hasWave() const { return (bool)wav; }
			};
			ProblemInfo info;
			Array<ChunkState> chunks;
			Optional<AnswerResponceData> answer = none;
		};

		SolverController(std::shared_ptr<SolverInterface> solver);

		void update();

		bool requestMatchInfo();
		bool requestProblemInfo();
		bool requestAnotherChunk(Optional<String> problemId = unspecified);

		Optional<MatchInfo> getMatchInfo();
		Optional<Array<String>> getProblemIds();
		Optional<ProblemState> getCurrentProblemState();
		Optional<ProblemState> getProblemState(String problemId);

		bool sendTheAnswer();


		// 試合情報が変更されたら solver を再起動しなければならない
		bool matchUpdated();
		void resetProblems();

		bool turnOnSolver();
		bool turnOffSolver();

		bool setCurrentProblem(String problemId);
		bool precalc(Array<String> languages);

		bool setServer(std::shared_ptr<AbstractServer> server);

		String getLastError() { return m_lastError; }
		std::shared_ptr<SolverInterface> getSolver() { return m_solver; }

	private:

		std::shared_ptr<SolverInterface> m_solver;
		std::shared_ptr<AbstractServer> m_serverInterface;
		Optional<MatchInfo> m_matchInfo = none;
		Array<ProblemState> m_problems;
		String m_lastError;
		String m_currentProblem;

		bool m_matchUpdated = false;

		int32 problemIndexFromId(String problemId);
		void chunkFound(String problemId, String filename);
		void seeChunks(String problemId);

	};


}
