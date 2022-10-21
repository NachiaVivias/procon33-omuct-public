#pragma once



#include <immintrin.h>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <cassert>

namespace FpsNyaan {


	namespace {



		constexpr int clzll(unsigned long long x) {
			int res = 0;
			if (x >> 32) { res += 32; x >>= 32; }
			if (x >> 16) { res += 16; x >>= 16; }
			if (x >> 8) { res += 8; x >>= 8; }
			if (x >> 4) { res += 4; x >>= 4; }
			if (x >> 2) { res += 2; x >>= 2; }
			if (x >> 1) { res += 1; x >>= 1; }
			return 31 - res;
		}

		constexpr int ctzll(unsigned long long x) {
			return 31 - clzll(x & (~x + 1));
		}

		constexpr int clz(unsigned int x) {
			int res = 0;
			if (x >> 16) { res += 16; x >>= 16; }
			if (x >> 8) { res += 8; x >>= 8; }
			if (x >> 4) { res += 4; x >>= 4; }
			if (x >> 2) { res += 2; x >>= 2; }
			if (x >> 1) { res += 1; x >>= 1; }
			return 63 - res;
		}

		constexpr int ctz(unsigned int x) {
			return 63 - clz(x & (~x + 1));
		}


		inline __m128i my128_mullo_epu32(
			const __m128i& a, const __m128i& b) {
			return _mm_mullo_epi32(a, b);
		}

		inline __m128i my128_mulhi_epu32(
			const __m128i& a, const __m128i& b) {
			__m128i a13 = _mm_shuffle_epi32(a, 0xF5);
			__m128i b13 = _mm_shuffle_epi32(b, 0xF5);
			__m128i prod02 = _mm_mul_epu32(a, b);
			__m128i prod13 = _mm_mul_epu32(a13, b13);
			__m128i prod = _mm_unpackhi_epi64(_mm_unpacklo_epi32(prod02, prod13),
				_mm_unpackhi_epi32(prod02, prod13));
			return prod;
		}

		inline __m128i montgomery_mul_128(
			const __m128i& a, const __m128i& b, const __m128i& r, const __m128i& m1) {
			return _mm_sub_epi32(
				_mm_add_epi32(my128_mulhi_epu32(a, b), m1),
				my128_mulhi_epu32(my128_mullo_epu32(my128_mullo_epu32(a, b), r), m1));
		}

		inline __m128i montgomery_add_128(
			const __m128i& a, const __m128i& b, const __m128i& m2, const __m128i& m0) {
			__m128i ret = _mm_sub_epi32(_mm_add_epi32(a, b), m2);
			return _mm_add_epi32(_mm_and_si128(_mm_cmpgt_epi32(m0, ret), m2), ret);
		}

		inline __m128i montgomery_sub_128(
			const __m128i& a, const __m128i& b, const __m128i& m2, const __m128i& m0) {
			__m128i ret = _mm_sub_epi32(a, b);
			return _mm_add_epi32(_mm_and_si128(_mm_cmpgt_epi32(m0, ret), m2), ret);
		}

		inline __m256i my256_mullo_epu32(
			const __m256i& a, const __m256i& b) {
			return _mm256_mullo_epi32(a, b);
		}

		inline __m256i my256_mulhi_epu32(
			const __m256i& a, const __m256i& b) {
			__m256i a13 = _mm256_shuffle_epi32(a, 0xF5);
			__m256i b13 = _mm256_shuffle_epi32(b, 0xF5);
			__m256i prod02 = _mm256_mul_epu32(a, b);
			__m256i prod13 = _mm256_mul_epu32(a13, b13);
			__m256i prod = _mm256_unpackhi_epi64(_mm256_unpacklo_epi32(prod02, prod13),
				_mm256_unpackhi_epi32(prod02, prod13));
			return prod;
		}

		inline __m256i montgomery_mul_256(
			const __m256i& a, const __m256i& b, const __m256i& r, const __m256i& m1) {
			return _mm256_sub_epi32(
				_mm256_add_epi32(my256_mulhi_epu32(a, b), m1),
				my256_mulhi_epu32(my256_mullo_epu32(my256_mullo_epu32(a, b), r), m1));
		}

		
		inline __m256i montgomery_reduction_256_2(
			const __m256i& a, const __m256i& r, const __m256i& m1) {
			return _mm256_sub_epi32(
				_mm256_add_epi32(a, m1),
				_mm256_mul_epu32(_mm256_mul_epu32(a, r), m1)
			);
		}

		inline __m256i montgomery_mul_256_2(
			const __m256i& a, const __m256i& b, const __m256i& r, const __m256i& m1) {
			__m256i tx = montgomery_reduction_256_2(_mm256_mul_epu32(_mm256_shuffle_epi32(a, 0xF5), _mm256_shuffle_epi32(b, 0xF5)), r, m1);
			__m256i ty = montgomery_reduction_256_2(_mm256_mul_epu32(a, b), r, m1);
			return _mm256_unpackhi_epi64(_mm256_unpacklo_epi32(ty, tx), _mm256_unpackhi_epi32(ty, tx));
		}
		

		inline __m256i montgomery_add_256(
			const __m256i& a, const __m256i& b, const __m256i& m2, const __m256i& m0) {
			__m256i ret = _mm256_sub_epi32(_mm256_add_epi32(a, b), m2);
			return _mm256_add_epi32(_mm256_and_si256(_mm256_cmpgt_epi32(m0, ret), m2),
				ret);
		}

		inline __m256i montgomery_sub_256(
			const __m256i& a, const __m256i& b, const __m256i& m2, const __m256i& m0) {
			__m256i ret = _mm256_sub_epi32(a, b);
			return _mm256_add_epi32(_mm256_and_si256(_mm256_cmpgt_epi32(m0, ret), m2),
				ret);
		}

	}

} // namespace FpsNyaan
