#include "match_virtual_server.h"


namespace Procon33 {

	namespace {

		const int32 NUM_CARDS = 44;
		const String SOURCE_PATH = U"wav/";

		String ReaderFileNameFormat(String lang, int32 idx) {
			return SOURCE_PATH + lang + ToString((idx + 1) / 10) + ToString((idx + 1) % 10) + U".wav";
		}

		Array<String> getSourceList(String lang) {
			Array<String> res;
			for (int32 i = 0; i < NUM_CARDS; i++) {
				res.push_back(ReaderFileNameFormat(lang, i));
			}
			return res;
		}

		// ids は zero based
		Array<int32> getSourceLengthList(String lang, Array<int32> ids) {
			auto fnames = getSourceList(lang);
			return ids.map([&](int32 id) -> int32 {
				return (int32)(WAVEDecoder().decode(fnames[id]).lengthSample());
			});
		}

	}


	ProblemSource ProblemSource::GetSample(String name) {
		ProblemSource res;
		res.problemName = name;
		res.chunkLength = { 30000, 30000 };
		res.readPos = { {.id = 0, .pos = -20000 }, {.id = 1, .pos = 0} };
		res.order = { 0, 1 };
		res.startTime = Time::GetSecSinceEpoch() + 120;
		res.timeLimit = 120;
		res.completeTrivial();
		return res;
	}

	ProblemSource ProblemSource::FromJson(JSON json) {
		ProblemSource res;
		res.problemName = json[U"problemName"].getString();
		res.chunkNum = json[U"chunkNum"].get<int32>();
		res.dataNum = json[U"dataNum"].get<int32>();
		for (JSON a : json[U"readPos"].arrayView()) {
			ReadPos readPos;
			readPos.id = a[U"id"].get<int32>();
			readPos.pos = a[U"pos"].get<int32>();
			res.readPos.push_back(readPos);
		}
		for (JSON a : json[U"chunkLength"].arrayView()) res.chunkLength.push_back(a.get<int32>());
		for (JSON a : json[U"order"].arrayView()) res.order.push_back(a.get<int32>());
		res.startTime = json[U"startTime"].get<uint64>();
		res.timeLimit = json[U"timeLimit"].get<uint64>();
		return res;
	}

	JSON ProblemSource::toJson() const {
		JSON json;
		json[U"problemName"] = problemName;
		json[U"chunkNum"] = chunkNum;
		json[U"dataNum"] = dataNum;
		json[U"readPos"] = readPos.map([](ReadPos v) -> JSON {
			JSON res;
			res[U"id"] = v.id;
			res[U"pos"] = v.pos;
			return res;
		});
		json[U"chunkLength"] = chunkLength.map([](int32 v) -> JSON { return JSON(v); });
		json[U"order"] = order.map([](int32 v) -> JSON { return JSON(v); });
		json[U"startTime"] = startTime;
		json[U"timeLimit"] = timeLimit;
		return json;
	}

	MatchSource MatchSource::GetSample() {
		MatchSource res;
		res.problems = { ProblemSource::GetSample() };
		res.penalty = 1;
		res.bonus = { 1.4, 1.3, 1.2, 1.1, 1.0 };
		res.language = U"J";
		return res;
	}

	MatchSource MatchSource::FromJson(JSON json) {
		MatchSource res;
		for (JSON a : json[U"problems"].arrayView()) res.problems.push_back(ProblemSource::FromJson(a));
		for (JSON a : json[U"bonus"].arrayView()) res.bonus.push_back(a.get<double>());
		res.penalty = json[U"penalty"].get<int32>();
		res.language = json[U"language"].getString();
		return res;
	}

	JSON MatchSource::toJson() const {
		JSON json;
		json[U"problems"] = problems.map([](const ProblemSource& v) -> JSON { return v.toJson(); });
		json[U"bonus"] = bonus.map([](double v) -> JSON { return JSON(v); });
		json[U"penalty"] = penalty;
		json[U"language"] = language;
		return json;
	}

	MatchSource MatchSource::Generate001(String matchName, String lang, int32 n, int32 length, double pl, double pr, SmallRNG& rng) {
		if (pl > pr) throw Error(U"pl > pr");
		ProblemSource problem;
		problem.chunkNum = 1;
		problem.dataNum = n;
		problem.order = { 0 };
		problem.problemName = matchName + U"-01";
		problem.chunkLength = { length };
		Array<int32> idxs = s3d::Range(0, NUM_CARDS - 1).asArray().choice(n, rng);
		Array<int32> lengthList = getSourceLengthList(lang, idxs);
		Array<int32> offs;
		for (int32 i = 0; i < n; i++) {
			int32 maxL = lengthList[i] - length;
			double p = UniformDistribution<double>(pl, pr)(rng);
			int32 off = -(int32)(maxL * p);
			problem.readPos.push_back({ idxs[i], off });
		}
		problem.setTime(Time::GetSecSinceEpoch(), 120).randomizeOrder(rng);
		MatchSource match = MatchSource::GetSample();
		match.language = lang;
		match.problems = { problem };
		match.trivialBonusFactor();
		return match;
	}

	MatchSource MatchSource::Generate002(String matchName, String lang, int32 n, int32 length, double pl, double pr, SmallRNG& rng) {
		if (pl > pr) throw Error(U"pl > pr");
		ProblemSource problem;
		problem.chunkNum = 1;
		problem.dataNum = n;
		problem.order = { 0 };
		problem.problemName = matchName + U"-01";
		problem.chunkLength = { length };
		Array<int32> idxs = s3d::Range(0, NUM_CARDS - 1).asArray().choice(n, rng);
		Array<int32> lengthList = getSourceLengthList(lang, idxs);
		Array<int32> offs;
		for (int32 i = 0; i < n; i++) {
			int32 maxL = lengthList[i] + length / 2;
			double p = UniformDistribution<double>(pl, pr)(rng);
			int32 off = -(int32)(maxL * p) + length * 3 / 4;
			problem.readPos.push_back({ idxs[i], off });
		}
		problem.setTime(Time::GetSecSinceEpoch(), 120).randomizeOrder(rng);
		MatchSource match = MatchSource::GetSample();
		match.language = lang;
		match.problems = { problem };
		match.trivialBonusFactor();
		return match;
	}

	MatchSource MatchSource::Generate003(String matchName, String lang, int32 chunkNum, int32 length, SmallRNG& rng) {
		MatchSource match = MatchSource::GetSample();
		match.language = lang;
		match.problems.clear();
		Array<int32> allIdxs = s3d::Range(0, NUM_CARDS - 1).asArray();
		for (int t = 0; t < 2; t++) {
			ProblemSource problem;
			problem.problemName = matchName + U"-0" + ToString(t + 1);
			problem.chunkLength = Array<int32>(chunkNum, length);
			allIdxs.shuffle(rng);
			Array<int32> idxs(allIdxs.end() - 20, allIdxs.end());
			allIdxs.erase(allIdxs.end() - 20, allIdxs.end());
			Array<int32> lengthList = getSourceLengthList(lang, idxs);
			for (int32 i = 0; i < 20; i++) {
				int32 maxL = lengthList[i] + length * (chunkNum - 1);
				double p = UniformDistribution<double>(0.0, 1.0)(rng);
				int32 off = (int32)(maxL * p) - lengthList[i] + length / 2;
				problem.readPos.push_back({ idxs[i], off });
			}
			problem.completeTrivial().setTime(Time::GetSecSinceEpoch() + 150 * t, 120).randomizeOrder(rng);
			match.problems.push_back(problem);
		}
		match.trivialBonusFactor();
		return match;
	}

	MatchSource MatchSource::Generate004(String matchName, String lang, SmallRNG& rng) {
		MatchSource match = MatchSource::GetSample();
		match.language = lang;
		match.problems.clear();
		Array<int32> allIdxs = s3d::Range(0, NUM_CARDS - 1).asArray();
		for (int t = 0; t < 10; t++) {
			ProblemSource problem;
			problem.problemName = matchName + U"-0" + ToString(t + 1);
			problem.chunkLength = Array<int32>(5, 30000);
			allIdxs.shuffle(rng);
			Array<int32> idxs(allIdxs.end() - 3, allIdxs.end());
			allIdxs.erase(allIdxs.end() - 3, allIdxs.end());
			Array<int32> lengthList = getSourceLengthList(lang, idxs);
			for (int32 i = 0; i < 3; i++) {
				int32 maxL = 20000;
				double p = UniformDistribution<double>(0.0, 1.0)(rng);
				int32 off = -(int32)(maxL * p);
				problem.readPos.push_back({ idxs[i], off });
			}
			problem.completeTrivial().setTime(Time::GetSecSinceEpoch() + 30 * t, 30).randomizeOrder(rng);
			match.problems.push_back(problem);
		}
		match.trivialBonusFactor();
		return match;
	}


	MatchSource MatchSource::GenerateCase(int32 caseid) {
		int oriid = caseid;
		SmallRNG rng(caseid);
		if (caseid-- == 0) return Generate001(U"random001", U"E", 20, 25000, 0.0, 1.0, rng);
		if (caseid-- == 0) return Generate001(U"random002", U"J", 20, 25000, 0.0, 1.0, rng);
		if (caseid-- == 0) return Generate001(U"random003", U"E", 20, 25000, 0.0, 0.1, rng);
		if (caseid-- == 0) return Generate001(U"random004", U"J", 20, 25000, 0.0, 0.1, rng);
		if (caseid-- == 0) return Generate001(U"random005", U"E", 20, 25000, 0.9, 1.0, rng);
		if (caseid-- == 0) return Generate001(U"random006", U"J", 20, 25000, 0.9, 1.0, rng);
		if (caseid-- == 0) return Generate001(U"random007", U"E", 20, 25000, 0.4, 0.6, rng);
		if (caseid-- == 0) return Generate001(U"random008", U"J", 20, 25000, 0.4, 0.6, rng);
		if (caseid-- == 0) return Generate002(U"random009", U"E", 20, 25000, 0.0, 1.0, rng);
		if (caseid-- == 0) return Generate002(U"random010", U"J", 20, 25000, 0.0, 1.0, rng);
		if (caseid-- == 0) return Generate002(U"random011", U"E", 20, 25000, 0.0, 0.1, rng);
		if (caseid-- == 0) return Generate002(U"random012", U"J", 20, 25000, 0.0, 0.1, rng);
		if (caseid-- == 0) return Generate002(U"random013", U"E", 20, 25000, 0.9, 1.0, rng);
		if (caseid-- == 0) return Generate002(U"random014", U"J", 20, 25000, 0.9, 1.0, rng);
		if (caseid-- == 0) return Generate002(U"random015", U"E", 20, 25000, 0.4, 0.6, rng);
		if (caseid-- == 0) return Generate002(U"random016", U"J", 20, 25000, 0.4, 0.6, rng);
		if (caseid-- == 0) return Generate003(U"random017", U"E", 15, 25000, rng);
		if (caseid-- == 0) return Generate003(U"random018", U"J", 15, 25000, rng);
		if (caseid-- == 0) return Generate004(U"random019", U"E", rng);
		if (caseid-- == 0) return Generate004(U"random020", U"J", rng);
		throw Error(U"no generate case assigned to the caseid = " + ToString(oriid));
	}

}
