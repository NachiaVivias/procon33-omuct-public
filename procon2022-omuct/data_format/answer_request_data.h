#pragma once

#include <Siv3D.hpp> // OpenSiv3D v0.6.4


namespace Procon33 {

	struct AnswerRequestData {
		String problemId; // 問題情報に書いてある
		Array<String> answers; // 読みデータの番号

		static AnswerRequestData fromJson(const JSON src) {
			AnswerRequestData buf;
			buf.problemId = src[U"problem_id"].get<String>();
			auto answerSrc = src[U"answers"];
			buf.answers.resize(answerSrc.size());
			for (size_t i = 0; i < buf.answers.size(); i++) {
				buf.answers[i] = answerSrc[i].get<String>();
			}
			return buf;
		}

		JSON toJson() const {
			JSON buf;
			buf[U"problem_id"] = problemId;
			buf[U"answers"] = answers.map([](const String& x) -> JSON { return JSON(x); });
			return buf;
		}

		std::string toJsonString() const {
			String ans = U"{\n";
			ans += U"    \"problem_id\": \"";
			ans += problemId;
			ans += U"\",\n";
			ans += U"    \"answers\": [";
			bool needComma = false;
			for (String s : answers) {
				if (needComma) ans += U", ";
				ans += U"\"";
				ans += s;
				ans += U"\"";
				needComma = true;
			}
			ans += U"]\n";
			ans += U"\n}";
			return ans.narrow();
		}

		String toJsonStringU32() const {
			String ans = U"{\n";
			ans += U"    \"problem_id\": \"";
			ans += problemId;
			ans += U"\",\n";
			ans += U"    \"answers\": [";
			bool needComma = false;
			for (String s : answers) {
				if (needComma) ans += U", ";
				ans += U"\"";
				ans += s;
				ans += U"\"";
				needComma = true;
			}
			ans += U"]\n";
			ans += U"\n}";
			return ans;
		}
	};

} // namespace Procon33
