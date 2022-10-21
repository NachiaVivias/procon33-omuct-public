
#include "wave_int32.h"


namespace Procon33 {



	// 0 埋め
	WaveInt32::WaveInt32(size_t length) {
		m_wav.assign(length, 0);
	}

	WaveInt32::WaveInt32(const Array<int32>& arr) : m_wav(arr) {}
	WaveInt32::WaveInt32(Array<int32>&& arr) : m_wav(std::move(arr)) {}

	size_t WaveInt32::length() const { return m_wav.size(); }

	const int32& WaveInt32::at(size_t index) const { return m_wav.at(index); }
	int32& WaveInt32::at(size_t index) { return m_wav.at(index); }
	const int32& WaveInt32::operator[](size_t at) const { return m_wav[at]; }
	int32& WaveInt32::operator[](size_t at) { return m_wav[at]; }

	const Array<int32>& WaveInt32::getArray() const { return m_wav; }

	// ステレオのうち左だけとる。
	WaveInt32 WaveInt32::FromSiv3dWaveLeft(const Wave& wav) {
		auto len = wav.lengthSample();
		WaveInt32 res(len);
		for (size_t t = 0; t < len; t++) {
			res[t] = wav.at(t).asWaveSampleS16().left;
		}
		return res;
	}

	// 急に右を取り出したくなったとき用。
	// 原則 FromSiv3dWaveLeft を使う。
	WaveInt32 WaveInt32::FromSiv3dWaveRight(const Wave& wav) {
		auto len = wav.lengthSample();
		WaveInt32 res(len);
		for (size_t t = 0; t < len; t++) {
			res[t] = wav.at(t).asWaveSampleS16().right;
		}
		return res;
	}

	// WAVE フォーマットのファイルから読み込む。
	WaveInt32 WaveInt32::FromWavFile(FilePathView path) {
		return FromSiv3dWaveLeft(Wave(path, AudioFormat::WAVE));
	}

	// WAVE フォーマットでファイルに出力する。
	// Siv3D の機能の制約で、ステレオ形式を出力する。
	void WaveInt32::saveWaveFile(FilePathView path) const {
		Wave trans(length());
		for (size_t t = 0; t < length(); t++) {
			trans.at(t) = WaveSampleS16(clampIntoInt16(m_wav[t])).asWaveSample();
		}
		trans.saveWAVE(path, WAVEFormat::StereoSint16);
	}

	// int16 に収まるように clamp する。
	// 値が変化した個数を返す。
	size_t WaveInt32::clampEachSampleIntoInt16() {
		size_t changeCount = 0;
		for (auto& a : m_wav) {
			int32 aa = clampIntoInt16(a);
			if (a != aa) changeCount++;
			a = aa;
		}
		return changeCount;
	}

	// 波形を 1 つ合成する。
	// src の先頭位置は、 this の先頭 + srcLatency に合わせる。
	// multiply は倍率（ -1 に設定すると引き算になる ）
	// 合成範囲が this の範囲からはみ出るならその部分は計算しない。
	void WaveInt32::addWave(const WaveInt32& src, int srcLatency, int multiply) {
		int srcRangeL = std::max(0, -srcLatency);
		int srcRangeR = std::min((int)src.length(), (int)this->length() - srcLatency);
		if (srcRangeL >= srcRangeR) return;
		for (int srcT = srcRangeL; srcT < srcRangeR; srcT++) {
			(*this)[srcT + srcLatency] += src[srcT] * multiply;
		}
	}

	// 上限・下限の値を数える
	size_t WaveInt32::countLikelyClamped() const {
		size_t res = 0;
		for (auto& sample : m_wav) {
			if (sample <= INT16_MIN || INT16_MAX <= sample) res++;
		}
		return res;
	}

}

