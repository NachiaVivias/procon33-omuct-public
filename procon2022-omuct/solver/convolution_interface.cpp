
#include "convolution_interface.h"
#include "garner_simd.h"


namespace Procon33 {


	const Modint::ModPreset ConvolutionInerfaceSimd::ModPreset0 = Modint::ModPreset::MakeFromConstMod<MOD0>();
	const Modint::ModPreset ConvolutionInerfaceSimd::ModPreset1 = Modint::ModPreset::MakeFromConstMod<MOD1>();

	ConvolutionInerfaceSimd::ConvolutionInerfaceSimd() :
		m_mem0(ModPreset0),
		m_mem1(ModPreset1)
	{}

	void ConvolutionInerfaceSimd::write(const Array<int64>& wave) {
		m_size = (uint32)wave.size();

		m_mem0.clear();
		m_mem1.clear();

		Array<uint32> wave0(wave.size());
		for (int i = 0; i < (int)wave.size(); i++) {
			wave0[i] = wave[i] % MOD0 + MOD0;
			if (wave0[i] >= MOD0) wave0[i] -= MOD0;
		}
		m_mem0.write(wave0);
		m_mem0.intoMontgomery();
		m_mem0.butterflyInMontgomery();

		Array<uint32> wave1(wave.size());
		for (int i = 0; i < (int)wave.size(); i++) {
			wave1[i] = wave[i] % MOD1 + MOD1;
			if (wave1[i] >= MOD1) wave1[i] -= MOD1;
		}
		m_mem1.write(wave1);
		m_mem1.intoMontgomery();
		m_mem1.butterflyInMontgomery();
	}

	void ConvolutionInerfaceSimd::multiply(const ConvolutionInerfaceSimd& r) {
		if (m_size == 0 || r.m_size == 0) {
			m_size = 0;
		}
		else {
			m_size = m_size + r.m_size - 1;
		}
		m_size = std::min<uint32>(m_size, r.m_mem0.length());

		m_mem0.multiplyInMontgomery(r.m_mem0);
		m_mem1.multiplyInMontgomery(r.m_mem1);
	}

	void ConvolutionInerfaceSimd::copyFrom(const ConvolutionInerfaceSimd& src) {
		memcpy(m_mem0.getPtr(), src.m_mem0.getPtr(), sizeof(uint32) * m_mem0.length());
		memcpy(m_mem1.getPtr(), src.m_mem1.getPtr(), sizeof(uint32) * m_mem1.length());
		m_size = src.m_size;
	}

	Array<int64> ConvolutionInerfaceSimd::read() {

		m_mem0.butterflyInvInMontgomery();
		m_mem1.butterflyInvInMontgomery();

		auto res = GarnerSimd(ModPreset0, ModPreset1).solve(m_mem0, m_mem1, m_size);
		Array<int64> resbuf(m_size);
		res.copyTo(resbuf);
		return resbuf;
	}

	void ConvolutionInerfaceSimd::read(Array<int64>& dest) {

		m_mem0.butterflyInvInMontgomery();
		m_mem1.butterflyInvInMontgomery();

		auto res = GarnerSimd(ModPreset0, ModPreset1).solve(m_mem0, m_mem1, m_size);
		if (dest.size() < m_size) dest.resize(m_size);
		res.copyTo(dest);
	}

	uint32 ConvolutionInerfaceSimd::size() const { return m_size; }


} // nanespace Procon33
