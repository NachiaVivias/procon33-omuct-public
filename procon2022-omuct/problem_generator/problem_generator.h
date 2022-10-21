#pragma once
#include "../stdafx.h"
#include <map>

namespace Procon33 {


	struct ProblemSource {
		struct ReadPos {
			int32 id;
			int32 pos;
		};

		String problemName;
		int32 chunkNum = 0;
		int32 dataNum = 0;
		Array<ReadPos> readPos;
		Array<int32> chunkLength;
		Array<int32> order;
		uint64 startTime;
		uint64 timeLimit;

		static ProblemSource GetSample(String name = U"undefined");
		static ProblemSource FromJson(JSON json);
		JSON toJson() const;

		ProblemSource& setTime(uint64 start, uint64 length) {
			startTime = start;
			timeLimit = length;
			return *this;
		}

		template<class URNG>
		ProblemSource& randomizeOrder(URNG& rng) {
			order = Range(0, chunkNum - 1).asArray().shuffled(rng);
			return *this;
		}

		ProblemSource& completeTrivial() {
			chunkNum = (int32)chunkLength.size();
			dataNum = (int32)readPos.size();
			return *this;
		}
	};


	struct MatchSource {
		Array<ProblemSource> problems;
		Array<double> bonus;
		int32 penalty = 0;
		String language;

		MatchSource& trivialBonusFactor() {
			int32 x = 5;
			for (auto a : problems) x = Max(x, a.chunkNum);
			bonus = Array<double>(x, 1.0);
			return *this;
		}

		static MatchSource GetSample();
		static MatchSource FromJson(JSON json);
		JSON toJson() const;

		static MatchSource Generate001(String matchName, String lang, int32 n, int32 length, double pl, double pr, SmallRNG& rng);
		static MatchSource Generate002(String matchName, String lang, int32 n, int32 length, double pl, double pr, SmallRNG& rng);
		static MatchSource Generate003(String matchName, String lang, int32 chunkNum, int32 length, SmallRNG& rng);
		static MatchSource Generate004(String matchName, String lang, SmallRNG& rng);
		static MatchSource GenerateCase(int32 caseid);
	};


}
