#pragma once


#include <Siv3D.hpp>

namespace Procon33 {

	// モノラルの音声波形を表現。
	// 長さと標本値、標本総数は管理するが、ビットレートは管理しない。
	// int32 の範囲で計算する。
	class WaveInt32 {

	public:

		static int16 clampIntoInt16(
			int32 x
		) {
			if (x < INT16_MIN) return INT16_MIN;
			if (x > INT16_MAX) return INT16_MAX;
			return (int16)x;
		}

		// 0 埋め
		WaveInt32(size_t length = 0);

		WaveInt32(const Array<int32>& arr);
		WaveInt32(Array<int32>&& arr);

		size_t length() const;

		const int32& at(size_t index) const;
		int32& at(size_t index);
		const int32& operator[](size_t at) const;
		int32& operator[](size_t at);

		const Array<int32>& getArray() const;

		// ステレオのうち左だけとる。
		static WaveInt32 FromSiv3dWaveLeft(const Wave& wav);

		// 急に右を取り出したくなったとき用。
		// 原則 FromSiv3dWaveLeft を使う。
		static WaveInt32 FromSiv3dWaveRight(const Wave& wav);

		// WAVE フォーマットのファイルから読み込む。
		static WaveInt32 FromWavFile(FilePathView path);

		// WAVE フォーマットでファイルに出力する。
		// Siv3D の機能の制約で、ステレオ形式を出力する。
		void saveWaveFile(FilePathView path) const;

		// int16 に収まるように clamp する。
		// 値が変化した個数を返す。
		size_t clampEachSampleIntoInt16();

		// 波形を 1 つ合成する。
		// src の先頭位置は、 this の先頭 + srcLatency に合わせる。
		// multiply は倍率（ -1 に設定すると引き算になる ）
		// 合成範囲が this の範囲からはみ出るならその部分は計算しない。
		void addWave(const WaveInt32& src, int srcLatency, int multiply = 1);

		// 上限・下限の値を数える
		size_t countLikelyClamped() const;

	private:

		Array<int32> m_wav;

	};

}

