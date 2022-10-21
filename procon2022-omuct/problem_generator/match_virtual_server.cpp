#include "match_virtual_server.h"
#include "../wave_int32.h"
#include "../HTTP_communication.h"

namespace Procon33 {

	namespace {

		const int32 NUM_CARDS = 44;
		const String SOURCE_PATH = U"wav/";

		String ReaderFileNameFormat(String lang, int32 idx) {
			return SOURCE_PATH + lang + ToString((idx + 1) / 10) + ToString((idx + 1) % 10) + U".wav";
		}



		class RealServerInterface : public AbstractServer {
		public:

			virtual Optional<MatchInfo> requestMatchInfo() { return HTTP::GetMatchInfo(); }
			virtual Optional<ProblemInfo> requestProblemInfo() { return HTTP::GetProblemInfo(); }
			virtual Optional<ChunksInfo> requestChunks(int32 n) { return HTTP::GetChunks(n); }
			virtual Optional<Wave> requestChunkWave(String fileName) { return HTTP::GetData(fileName); }
			virtual Optional<AnswerResponceData> sendAnswer(const AnswerRequestData& answer) { return HTTP::SendAnswer(answer.toJsonString()); }

		};


		class VirtualMatchServer : public AbstractServer {
		private:

			void load(FilePathView jsonFilePath);
			void write(FilePathView jsonFilePath);

			int32 getProblemNumber();
			void generateWave(String problemId, int32 chunkIndex);
			String getChunkFileName(String problemId, int32 chunkIndex);
			Wave getChunkWave(String problemId, int32 chunkIndex);

			int32 problemIdFromTime(uint64 t);

			MatchInfo getMatchInfo();

			// -1 を指定すると現在時刻にしたがって問題を取得
			Optional<ProblemInfo> getProblemInfo(int32 id);

			MatchSource m_match;
			std::map<std::pair<String, int32>, std::pair<Wave, String>> wavMemo;

		public:

			VirtualMatchServer(const MatchSource& matchSource) : m_match(matchSource) {}
			virtual Optional<MatchInfo> requestMatchInfo() {
				return getMatchInfo();
			}
			virtual Optional<ProblemInfo> requestProblemInfo() {
				return getProblemInfo(-1);
			}
			virtual Optional<ChunksInfo> requestChunks(int32 n) {
				ChunksInfo res;
				auto problemInfo = getProblemInfo(-1);
				if (!problemInfo.has_value()) return none;
				for (int32 i = 0; i < n; i++) res.fileNames.push_back(getChunkFileName(problemInfo->problemId, i));
				return res;
			}
			virtual Optional<Wave> requestChunkWave(String fileName) {
				auto problemInfo = getProblemInfo(-1);
				if (!problemInfo.has_value()) return none;
				int32 chunkId = -1;
				for (int32 i = 0; i < problemInfo->chunksNum; i++) {
					if (fileName == getChunkFileName(problemInfo->problemId, i)){
						chunkId = i;
					}
				}
				if (chunkId < 0) return none;
				return getChunkWave(problemInfo->problemId, chunkId);
			}
			virtual Optional<AnswerResponceData> sendAnswer(const AnswerRequestData& answer) {
				AnswerResponceData res;
				res.problemId = answer.problemId;
				res.acceptTime = Time::GetSecSinceEpoch();
				res.answers = answer.answers;
				return res;
			}

		};

	}

	std::shared_ptr<AbstractServer> AbstractServer::GetRealServerInterface() {
		return std::make_shared<RealServerInterface>();
	}

	std::shared_ptr<AbstractServer> AbstractServer::GetVirtualMatchServer(const MatchSource& matchSource) {
		return std::make_shared<VirtualMatchServer>(matchSource);
	}

	std::shared_ptr<AbstractServer> AbstractServer::GetVirtualMatchServerById(int32 sampleMatchId) {
		auto src = MatchSource::GenerateCase(sampleMatchId);
		return GetVirtualMatchServer(src);
	}

	int32 VirtualMatchServer::getProblemNumber() { return static_cast<int32>(m_match.problems.size()); }

	void VirtualMatchServer::generateWave(String problemId, int32 chunkIndex) {
		if (wavMemo.count({ problemId, chunkIndex })) return;

		// 基礎情報
		int32 arrayIndex = -1;
		for (int32 i = 0; i < (int32)m_match.problems.size(); i++) if (m_match.problems[i].problemName == problemId) arrayIndex = i;
		auto& problem = m_match.problems[arrayIndex];
		int32 ord = problem.order[chunkIndex];
		int32 tl = 0;
		for (int32 i = 0; i < ord; i++) tl += problem.chunkLength[i];
		int32 tr = tl + problem.chunkLength[ord];

		// 問題波形を作る
		WaveInt32 waveBuf(tr - tl);
		for (auto r : problem.readPos) {
			FilePath readFilePath = ReaderFileNameFormat(m_match.language, r.id);
			auto pwave = WaveInt32::FromWavFile(readFilePath);
			waveBuf.addWave(pwave, r.pos - tl);
		}
		waveBuf.clampEachSampleIntoInt16();

		std::pair<Wave, String> res;
		res.first = Wave(waveBuf.length());
		for (size_t t = 0; t < waveBuf.length(); t++) res.first[t] = WaveSample::FromInt16(WaveInt32::clampIntoInt16(waveBuf[t]));
		auto md5 = MD5::FromBinary(res.first.data(), res.first.size() * sizeof(WaveSample));
		res.second = U"problem" + ToString(ord + 1) + U"_" + md5.asString();

		wavMemo[{ problemId, chunkIndex }] = std::move(res);
	}

	Wave VirtualMatchServer::getChunkWave(String problemId, int32 chunkIndex) {
		generateWave(problemId, chunkIndex);
		return wavMemo[{ problemId, chunkIndex }].first;
	}

	String VirtualMatchServer::getChunkFileName(String problemId, int32 chunkIndex) {
		generateWave(problemId, chunkIndex);
		return wavMemo[{ problemId, chunkIndex }].second;
	}

	int32 VirtualMatchServer::problemIdFromTime(uint64 t) {
		int32 res = -1;
		for (int32 i = 0; i < (int32)m_match.problems.size(); i++) {
			uint64 tl = m_match.problems[i].startTime;
			uint64 tr = tl + m_match.problems[i].timeLimit;
			if (tl <= t && t < tr) res = i;
		}
		return res;
	}

	MatchInfo VirtualMatchServer::getMatchInfo() {
		MatchInfo res;
		res.bonusFactor = m_match.bonus;
		res.changePenality = m_match.penalty;
		res.problemsNum = (int32)m_match.problems.size();
		return res;
	}

	Optional<ProblemInfo> VirtualMatchServer::getProblemInfo(int32 id) {
		if (id < 0) id = problemIdFromTime(Time::GetSecSinceEpoch());
		if (id < 0) return none;
		ProblemInfo res;
		auto& problem = m_match.problems[id];
		res.chunksNum = problem.chunkNum;
		res.dataNum = problem.dataNum;
		res.problemId = problem.problemName;
		res.startTime = problem.startTime;
		res.timeLimit = (int32)problem.timeLimit;
		return res;
	}

}
