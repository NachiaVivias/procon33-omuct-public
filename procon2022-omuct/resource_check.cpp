
#include "resource_check.h"

namespace Procon33 {

	void EnsureResourceFileList() {

		const String resourceFileListJson = U"resource_file_list.json";
		if (!FileSystem::IsFile(resourceFileListJson)) {
			throw Error(U"必要なファイルが見つかりません ： \"" + resourceFileListJson + U"\"");
		}

		JSON sourceList = JSON::Load(resourceFileListJson, YesNo<AllowExceptions_tag>::Yes);

		for (const auto& content : sourceList.arrayView()) {
			String path = content[U"path"].getString();
			if (!FileSystem::IsFile(path)) {
				throw Error(U"必要なファイルが見つかりません ： \"" + path + U"\"");
			}
			if (content[U"md5"].getString() != MD5::FromFile(path).asString()) {
				throw Error(U"必要なファイルの MD5 が一致しません ： \"" + path + U"\"");
			}
		}
	}

} // namespace Procon33
