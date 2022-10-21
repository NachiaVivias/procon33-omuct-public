#pragma once

#include <Siv3D.hpp> // OpenSiv3D v0.6.4


namespace Procon33 {

	struct AnswerResponceData {
		String problemId; // 問題情報に書いてある
		Array<String> answers; // 読みデータの番号
		int64 acceptTime = -1;

		static AnswerResponceData fromJson(const JSON src) {
			AnswerResponceData buf;
			buf.problemId = src[U"problem_id"].get<String>();
			auto answerSrc = src[U"answers"];
			buf.answers.resize(answerSrc.size());
			for (size_t i = 0; i < buf.answers.size(); i++) {
				buf.answers[i] = answerSrc[i].get<String>();
			}
			buf.acceptTime = src[U"accepted_at"].get<int64>();
			return buf;
		}

		JSON toJson() const {
			JSON buf;
			buf[U"problem_id"] = problemId;
			buf[U"answers"] = answers.map([](const String& x) -> JSON { return JSON(x); });
			buf[U"accepted_at"] = acceptTime;
			return buf;
		}
	};

} // namespace Procon33
