#pragma once


#include "montgomery_modint_info.h"
#include "fps_nyaan_modify.h"


namespace Procon33 {


	// 畳み込みを超簡単なインターフェースでやる
	// 長さが 524288 固定なので入力配列の長さにかかわらず 数十ミリ秒 かかる
	// 小規模な実験以外は Release ビルドすること
	//
	// 答えの各要素の絶対値が 4 × 10^17 以下になるとき正しく計算できる
	class ConvolutionInerfaceSimd {
	public:

		// このタイミングでメモリを確保する
		ConvolutionInerfaceSimd();

		// この構造体に配列を書き込む
		// 同時に FFT するので計算量が重い
		void write(const Array<int64>& wave);

		// 掛け算する
		// 計算量は小さい
		void multiply(const ConvolutionInerfaceSimd& r);

		// コピー
		void copyFrom(const ConvolutionInerfaceSimd& src);

		// この構造体から配列を読み出す
		// 同時に FFT するので計算量が重い
		Array<int64> read();
		void read(Array<int64>& dest);

		uint32 size() const;

	private:

		static const unsigned int MOD0 = 998244353;
		static const unsigned int MOD1 = 924844033;

		static const Modint::ModPreset ModPreset0;
		static const Modint::ModPreset ModPreset1;

		FpsNyaan::ModintVector m_mem0;
		FpsNyaan::ModintVector m_mem1;
		uint32 m_size = 0;
	};

}

