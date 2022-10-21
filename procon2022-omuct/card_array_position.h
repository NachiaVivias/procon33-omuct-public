#pragma once

#include <Siv3D.hpp>

namespace Procon33 {

	// cardCount 個の正方形を globalRect に並べるように配置して返す
	// -----------
	// | 0 1 2 3 |
	// | 4 5 6   |
	// -----------
	// 無理なら空の配列を返す
	Array<RectF> CardArrayPosition(
		RectF globalRect,
		uint32 cardCount
	) {
		if (globalRect.w <= 0.0 || globalRect.h <= 0.0) return {}; // 無理
		double ng = Min(globalRect.h, globalRect.w); // 細すぎて 1 つも入らない
		double ok = Min(ng, Max(globalRect.h, globalRect.w) / cardCount); // 一列に並ぶので ok
		if (ok > ng) return {}; // 無理
		constexpr double MAX_UINT32 = static_cast<double>(std::numeric_limits<uint32>::max());
		// 指数を二分探索
		while (ng / ok > 1.001) {
			double mid = Sqrt(ng * ok);
			uint32 rowSize = static_cast<uint32>(Floor(Min(globalRect.w / mid, MAX_UINT32)));
			uint32 colSize = static_cast<uint32>(Floor(Min(globalRect.h / mid, MAX_UINT32)));
			bool midOk = static_cast<uint64>(rowSize) * colSize >= cardCount;
			(midOk ? ok : ng) = mid;
		}

		// 上から順に埋める
		// 各行では左から順に埋める
		Array<RectF> result;
		uint32 rowSize = static_cast<uint32>(Floor(Min(globalRect.w / ok, MAX_UINT32)));
		uint32 colSize = static_cast<uint32>(Floor(Min(globalRect.h / ok, MAX_UINT32)));
		RectF::size_type rectSize(ok, ok);
		for (uint32 y = 0; y < colSize; y++) {
			if (result.size() >= cardCount) break;
			for (uint32 x = 0; x < rowSize; x++) {
				if (result.size() >= cardCount) break;
				result.push_back(RectF(globalRect.tl() + rectSize * RectF::size_type(x, y), rectSize));
			}
		}

		return result;
	}


	// CardArrayPosition の結果をテストする用
	// サンプルを描画する
	void CardArrayPositionDrawSample(
		Array<RectF> result,
		Font font
	) {
		for (size_t i = 0; i < result.size(); i++) {
			result[i].drawFrame();
			font(ToString(i)).drawAt(result[i].center());
		}
	}

} // namespace Procon33
