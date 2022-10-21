#pragma once

namespace Procon33 {

	// 断片のファイルの名前から断片の番号を取得する。
	//        正確には、 (数字以外)整数(数字以外)... の形の文字列であることを家庭紙、整数の部分を取り出す（そして 1 を引く）
	// 正常なら zero based の番号、無効なら -1 を返す。
	int32 GetIndexFromChunkFileName(String x);

}
