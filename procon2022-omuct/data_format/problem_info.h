#pragma once

#include<Siv3D.hpp>

namespace Procon33 {

	struct ProblemInfo {

		String problemId; //問題ID
		int32 chunksNum = -1; //分割の数
		int64 startTime = -1; //開始時刻
		int32 timeLimit = -1; //制限時間
		int32 dataNum = -1; //データの数

		static ProblemInfo FromJson(JSON src) {
			ProblemInfo buf;
			buf.problemId = src[U"id"].getString();
			buf.chunksNum = src[U"chunks"].get<int32>();
			buf.startTime = src[U"start_at"].get<int64>();
			buf.timeLimit = src[U"time_limit"].get<int32>();
			buf.dataNum = src[U"data"].get<int32>();
			return buf;
		}
	};


	

}
