
#include "garner_simd.h"

#include "fps_nyaan_utility.h"


namespace Procon33 {


	namespace{

		uint32 InvAInModBMontgomery(uint32 a, const Modint::ModPreset& b) {
			return b.MontgomeryInv(b.IntoMontgomery(a % b.MOD));
		}

	}


	GarnerSimd::GarnerSimd(Modint::ModPreset MOD0, Modint::ModPreset MOD1) :
		m_mod0(MOD0),
		m_mod1(MOD1),
		m_invMod0InMod1Montgomery(InvAInModBMontgomery(MOD0.MOD, MOD1)),
		m_invMod1InMod0Montgomery(InvAInModBMontgomery(MOD1.MOD, MOD0))
	{
	}

	FpsNyaan::AlignedArray<int64> GarnerSimd::solve(
		const FpsNyaan::ModintVector& a0,
		const FpsNyaan::ModintVector& a1,
		Optional<uint32> nSetting
	) {
		uint32 n = nSetting.value_or(std::min(a0.length(), a1.length()));
		if (n > a0.length() || n > a1.length()) throw Error(U"GarnerSimd::solve : length not match");
		FpsNyaan::AlignedArray<int64> res(n);
		res.malloc();

		uint32* p0 = a0.getPtr();
		uint32* p1 = a1.getPtr();
		int64* q = res.getPtr();

		int64 halfMod = ((int64)m_mod0.MOD * m_mod1.MOD) / 2;
		int64 allMod = (int64)m_mod0.MOD * m_mod1.MOD;

		__m256i m0m1Fill = _mm256_set1_epi32(m_invMod0InMod1Montgomery);
		__m256i m1m0Fill = _mm256_set1_epi32(m_invMod1InMod0Montgomery);
		__m256i mod0Fill = _mm256_set1_epi32(m_mod0.MOD);
		__m256i mod0rFill = _mm256_set1_epi32(m_mod0.R);
		__m256i mod1Fill = _mm256_set1_epi32(m_mod1.MOD);
		__m256i mod1rFill = _mm256_set1_epi32(m_mod1.R);
		__m256i oneFill = _mm256_set1_epi32(1);
		__m256i halfModFill = _mm256_set1_epi64x(halfMod);
		__m256i allModFill = _mm256_set1_epi64x(allMod);

		__m256i permutation04152637 = _mm256_set_epi32(7, 3, 6, 2, 5, 1, 4, 0);

		uint32 len8 = n / 8 * 8;
		for (uint32 i = 0; i < len8; i += 8) {

			__m256i p0Val = _mm256_loadu_si256((__m256i*)(p0 + i));
			p0Val = FpsNyaan::montgomery_mul_256(p0Val, m1m0Fill, mod0rFill, mod0Fill);
			p0Val = FpsNyaan::montgomery_mul_256(p0Val, oneFill, mod0rFill, mod0Fill);
			p0Val = _mm256_permutevar8x32_epi32(p0Val, permutation04152637);

			__m256i p1Val = _mm256_loadu_si256((__m256i*)(p1 + i));
			p1Val = FpsNyaan::montgomery_mul_256(p1Val, m0m1Fill, mod1rFill, mod1Fill);
			p1Val = FpsNyaan::montgomery_mul_256(p1Val, oneFill, mod1rFill, mod1Fill);
			p1Val = _mm256_permutevar8x32_epi32(p1Val, permutation04152637);

			__m256i mul0_0123 = _mm256_mul_epi32(p0Val, mod1Fill);
			__m256i mul1_0123 = _mm256_mul_epi32(p1Val, mod0Fill);
			__m256i res0123 = _mm256_add_epi64(mul0_0123, mul1_0123);
			__m256i mul0_4567 = _mm256_mul_epi32(_mm256_srli_epi64(p0Val, 32), mod1Fill);
			__m256i mul1_4567 = _mm256_mul_epi32(_mm256_srli_epi64(p1Val, 32), mod0Fill);
			__m256i res4567 = _mm256_add_epi64(mul0_4567, mul1_4567);

			res0123 = _mm256_sub_epi64(res0123, _mm256_and_si256(_mm256_cmpgt_epi64(res0123, halfModFill), allModFill));
			res0123 = _mm256_sub_epi64(res0123, _mm256_and_si256(_mm256_cmpgt_epi64(res0123, halfModFill), allModFill));
			res4567 = _mm256_sub_epi64(res4567, _mm256_and_si256(_mm256_cmpgt_epi64(res4567, halfModFill), allModFill));
			res4567 = _mm256_sub_epi64(res4567, _mm256_and_si256(_mm256_cmpgt_epi64(res4567, halfModFill), allModFill));

			_mm256_storeu_si256((__m256i*)(q + i), res0123);
			_mm256_storeu_si256((__m256i*)(q + i + 4), res4567);
		}

		for (uint32 i = len8; i < n; i++) {
			int64 q0 = m_mod0.MontgomeryReduction(m_mod0.MontgomeryMultiply(p0[i], m_invMod1InMod0Montgomery));
			int64 q1 = m_mod1.MontgomeryReduction(m_mod1.MontgomeryMultiply(p1[i], m_invMod0InMod1Montgomery));
			int64 resVal = q0 * m_mod1.MOD + q1 * m_mod0.MOD;
			if (resVal > halfMod) resVal -= allMod;
			if (resVal > halfMod) resVal -= allMod;
			res.getPtr()[i] = resVal;
		}

		return res;
	}



} // namespace Procon33
