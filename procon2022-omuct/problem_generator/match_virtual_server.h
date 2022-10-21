#pragma once
#include "../stdafx.h"
#include "problem_generator.h"
#include "../data_format.h"
#include "../wave_int32.h"
#include "../chunk_idx_from_filename.h"
#include <map>

namespace Procon33 {


	class AbstractServer {
	public:

		virtual ~AbstractServer() = default;
		virtual Optional<MatchInfo> requestMatchInfo() = 0;
		virtual Optional<ProblemInfo> requestProblemInfo() = 0;
		virtual Optional<ChunksInfo> requestChunks(int32 n) = 0;
		virtual Optional<Wave> requestChunkWave(String fileName) = 0;
		virtual Optional<AnswerResponceData> sendAnswer(const AnswerRequestData& answer) = 0;

		static std::shared_ptr<AbstractServer> GetRealServerInterface();
		static std::shared_ptr<AbstractServer> GetVirtualMatchServer(const MatchSource& matchSource);
		static std::shared_ptr<AbstractServer> GetVirtualMatchServerById(int32 sampleMatchId);
	};


	class ProblemManager {
		Array<String> m_chunkFiles;
		Array<WaveInt32> m_chunkWaves;
	public:

		ProblemManager(ProblemInfo& problem) {
			m_chunkFiles.resize(problem.chunksNum);
			m_chunkWaves.resize(problem.chunksNum);
		}

		// 新しければ断片の番号、そうでなければ -1 を返す。
		int giveChunkFile(String filepath) {
			int32 k = GetIndexFromChunkFileName(FileSystem::FileName(filepath));
			if (k == -1) throw Error(U"ProblemManager::giveChunk のエラー ： 断片のファイル名から番号を取得できませんでした。");
			if (k >= (int32)m_chunkFiles.size()) throw Error(U"ProblemManager::giveChunk のエラー ： 番号が範囲外の断片です。");
			if (m_chunkFiles[k] == filepath) return -1;
			m_chunkFiles[k] = filepath;
			m_chunkWaves[k] = WaveInt32::FromWavFile(filepath);
			return k;
		}

		const WaveInt32& getChunkWave(int32 k) {
			return m_chunkWaves[k];
		}

		int32 countGivenChunks() {
			int32 ans = 0;
			for (String& s : m_chunkFiles) if (s != U"") ans++;
			return ans;
		}
	};


	class MatchManager {
		std::map<String, ProblemManager> m_problems;
	public:
		ProblemManager& getProblemManager(String problemId) { return m_problems.at(problemId); }

		// 新しいなら true
		bool newProblem(ProblemInfo problem) {
			if (m_problems.count(problem.problemId)) return false;
			m_problems.emplace(problem.problemId, ProblemManager(problem));
			return true;
		}
	};

}
