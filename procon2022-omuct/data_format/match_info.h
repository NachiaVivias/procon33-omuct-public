#pragma once

#include <Siv3D.hpp> // OpenSiv3D v0.6.4


namespace Procon33 {


	struct MatchInfo {
		int32 problemsNum; // 試合に含まれる問題の個数
		Array<double> bonusFactor; // ボーナス係数
		int32 changePenality; // 変更札のペナルティ
	};


} // namespace Procon33

