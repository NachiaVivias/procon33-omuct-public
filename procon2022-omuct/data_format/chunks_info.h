#pragma once

#include <Siv3D.hpp> // OpenSiv3D v0.6.4


namespace Procon33 {

	struct ChunksInfo {
		Array<String> fileNames; // リクエストに必要なファイル名

		static ChunksInfo fromJson(const JSON src) {
			ChunksInfo buf;
			auto fileNamesSrc = src[U"chunks"];
			buf.fileNames.resize(fileNamesSrc.size());
			for (size_t i = 0; i < buf.fileNames.size(); i++) {
				buf.fileNames[i] = fileNamesSrc[i].get<String>();
			}
			return buf;
		}

		JSON toJson() const {
			JSON buf;
			buf[U"chunks"] = fileNames.map([](const String& x) -> JSON { return JSON(x); });
			return buf;
		}
	};

} // namespace Procon33
