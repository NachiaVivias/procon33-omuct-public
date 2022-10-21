#pragma once

#include <cmath>

namespace Procon33 {


	// [0.0, 1.0] のスライダで整数を設定するための変換式を管理する。
	class IntegerSlider {
	private:
		long long m_rangeMin;
		long long m_rangeMax;
		long long m_val;

		double valueWidth() const {
			return static_cast<double>(m_rangeMax - m_rangeMin);
		}
	public:

		// [l, r] 内で設定するスライダを作る。
		// 初期値は defaultVal で設定する。
		IntegerSlider(long long l, long long r, long long defaultVal) {
			m_rangeMin = l;
			m_rangeMax = r;
			m_val = defaultVal;
		}

		// スライダの位置を表す [0.0, 1.0] の値を取得。
		double getSliderPos() const {
			return static_cast<double>(m_val - m_rangeMin) / valueWidth();
		}

		// スライダの位置を sliderPos に設定する。
		void updateSliderPos(double sliderPos) {
			m_val = m_rangeMin + static_cast<long long>(std::round(sliderPos * valueWidth()));
		}

		// 設定された整数を取得。
		long long getVal() const {
			return m_val;
		}
	};

}

