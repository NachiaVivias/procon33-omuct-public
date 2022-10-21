
#include "fps_nyaan_modify.h"

#include "fps_nyaan_utility.h"

namespace FpsNyaan {


	namespace ntt_inner {

		namespace {

			constexpr int SZ_FFT_BUF = 1 << 23;

			__declspec(align(64)) std::uint32_t _buf1[SZ_FFT_BUF];
			__declspec(align(64)) std::uint32_t _buf2[SZ_FFT_BUF];

		}

	}  // namespace ntt_inner

	namespace {



		struct NTT {
		private:
			const Modint::ModPreset m_modPreset;
			const uint32 mod;
			const uint32 pr;
			const uint32 level;

			uint32* dw;
			uint32* dy;
			uint32* buf1;
			uint32* buf2;

			uint32 add(uint32 a, uint32 b) {
				uint32 res = a + b;
				if (res >= mod * 2) res -= mod << 1;
				return res;
			}
			uint32 sub(uint32 a, uint32 b) {
				uint32 res = a + mod * 2 - b;
				if (res >= mod * 2) res -= mod * 2;
				return res;
			}
			uint32 mul(uint32 a, uint32 b) { return m_modPreset.MontgomeryMultiply(a, b); }
			uint32 pow(uint32 a, uint32 b) { return m_modPreset.MontgomeryPow(a, b); }
			uint32 inv(uint32 a) { return m_modPreset.MontgomeryInv(a); }


			void setwy(int k) {
				dw = (uint32*)malloc(sizeof(uint32) * level);
				dy = (uint32*)malloc(sizeof(uint32) * level);
				Array<uint32> w(level);
				Array<uint32> y(level);
				w[k - 1] = pow(m_modPreset.IntoMontgomery(pr), (mod - 1) / (1 << k));
				y[k - 1] = inv(w[k - 1]);
				for (int i = k - 2; i > 0; --i) {
					w[i] = mul(w[i + 1], w[i + 1]);
					y[i] = mul(y[i + 1], y[i + 1]);
				}
				dw[0] = dy[0] = mul(w[1], w[1]);
				dw[1] = w[1], dy[1] = y[1], dw[2] = w[2], dy[2] = y[2];
				for (int i = 3; i < k; ++i) {
					dw[i] = mul(mul(dw[i - 1], y[i - 2]), w[i]);
					dy[i] = mul(mul(dy[i - 1], w[i - 2]), y[i]);
				}
			}

			NTT(Modint::ModPreset modPreset) :
				m_modPreset(modPreset),
				mod(modPreset.MOD),
				pr(modPreset.PRIMITIVE_ROOT),
				level(ctzll(modPreset.MOD - 1))
			{
				setwy(level);
				buf1 = ntt_inner::_buf1;
				buf2 = ntt_inner::_buf2;
			}

		public:

			~NTT() {
				free(dw);
				free(dy);
			}

			void ntt(uint32* a, int n) {
				int k = n ? ctz(n) : 0;
				if (k == 0) return;
				if (k == 1) {
					uint32 a1 = a[1];
					a[1] = sub(a[0], a[1]);
					a[0] = add(a[0], a1);
					return;
				}
				if (k & 1) {
					int v = 1 << (k - 1);
					if (v < 8) {
						for (int j = 0; j < v; ++j) {
							uint32 ajv = a[j + v];
							a[j + v] = sub(a[j], ajv);
							a[j] = add(a[j], ajv);
						}
					}
					else {
						const __m256i m0 = _mm256_set1_epi32(0);
						const __m256i m2 = _mm256_set1_epi32(mod + mod);
						int j0 = 0;
						int j1 = v;
						for (; j0 < v; j0 += 8, j1 += 8) {
							__m256i T0 = _mm256_loadu_si256((__m256i*)(a + j0));
							__m256i T1 = _mm256_loadu_si256((__m256i*)(a + j1));
							__m256i naj = montgomery_add_256(T0, T1, m2, m0);
							__m256i najv = montgomery_sub_256(T0, T1, m2, m0);
							_mm256_storeu_si256((__m256i*)(a + j0), naj);
							_mm256_storeu_si256((__m256i*)(a + j1), najv);
						}
					}
				}
				int u = 1 << (2 + (k & 1));
				int v = 1 << (k - 2 - (k & 1));
				uint32 one = m_modPreset.IntoMontgomery(1);
				uint32 imag = dw[1];
				while (v) {
					if (v == 1) {
						uint32 ww = one, xx = one, wx = one;
						for (int jh = 0; jh < u;) {
							ww = mul(xx, xx), wx = mul(ww, xx);
							uint32 t0 = a[jh + 0], t1 = mul(a[jh + 1], xx);
							uint32 t2 = mul(a[jh + 2], ww), t3 = mul(a[jh + 3], wx);
							uint32 t0p2 = add(t0, t2), t1p3 = add(t1, t3);
							uint32 t0m2 = sub(t0, t2), t1m3 = mul(sub(t1, t3), imag);
							a[jh + 0] = add(t0p2, t1p3), a[jh + 1] = sub(t0p2, t1p3);
							a[jh + 2] = add(t0m2, t1m3), a[jh + 3] = sub(t0m2, t1m3);
							xx = mul(xx, dw[ctz((jh += 4))]);
						}
					}
					else if (v == 4) {
						const __m128i m0 = _mm_set1_epi32(0);
						const __m128i m1 = _mm_set1_epi32(mod);
						const __m128i m2 = _mm_set1_epi32(mod + mod);
						const __m128i r = _mm_set1_epi32(m_modPreset.R);
						const __m128i Imag = _mm_set1_epi32(imag);
						uint32 ww = one, xx = one, wx = one;
						for (int jh = 0; jh < u;) {
							if (jh == 0) {
								int j0 = 0;
								int j1 = v;
								int j2 = j1 + v;
								int j3 = j2 + v;
								int je = v;
								for (; j0 < je; j0 += 4, j1 += 4, j2 += 4, j3 += 4) {
									const __m128i T0 = _mm_loadu_si128((__m128i*)(a + j0));
									const __m128i T1 = _mm_loadu_si128((__m128i*)(a + j1));
									const __m128i T2 = _mm_loadu_si128((__m128i*)(a + j2));
									const __m128i T3 = _mm_loadu_si128((__m128i*)(a + j3));
									const __m128i T0P2 = montgomery_add_128(T0, T2, m2, m0);
									const __m128i T1P3 = montgomery_add_128(T1, T3, m2, m0);
									const __m128i T0M2 = montgomery_sub_128(T0, T2, m2, m0);
									const __m128i T1M3 = montgomery_mul_128(
										montgomery_sub_128(T1, T3, m2, m0), Imag, r, m1);
									_mm_storeu_si128((__m128i*)(a + j0),
										montgomery_add_128(T0P2, T1P3, m2, m0));
									_mm_storeu_si128((__m128i*)(a + j1),
										montgomery_sub_128(T0P2, T1P3, m2, m0));
									_mm_storeu_si128((__m128i*)(a + j2),
										montgomery_add_128(T0M2, T1M3, m2, m0));
									_mm_storeu_si128((__m128i*)(a + j3),
										montgomery_sub_128(T0M2, T1M3, m2, m0));
								}
							}
							else {
								ww = mul(xx, xx), wx = mul(ww, xx);
								const __m128i WW = _mm_set1_epi32(ww);
								const __m128i WX = _mm_set1_epi32(wx);
								const __m128i XX = _mm_set1_epi32(xx);
								int j0 = jh * v;
								int j1 = j0 + v;
								int j2 = j1 + v;
								int j3 = j2 + v;
								int je = j1;
								for (; j0 < je; j0 += 4, j1 += 4, j2 += 4, j3 += 4) {
									const __m128i T0 = _mm_loadu_si128((__m128i*)(a + j0));
									const __m128i T1 = _mm_loadu_si128((__m128i*)(a + j1));
									const __m128i T2 = _mm_loadu_si128((__m128i*)(a + j2));
									const __m128i T3 = _mm_loadu_si128((__m128i*)(a + j3));
									const __m128i MT1 = montgomery_mul_128(T1, XX, r, m1);
									const __m128i MT2 = montgomery_mul_128(T2, WW, r, m1);
									const __m128i MT3 = montgomery_mul_128(T3, WX, r, m1);
									const __m128i T0P2 = montgomery_add_128(T0, MT2, m2, m0);
									const __m128i T1P3 = montgomery_add_128(MT1, MT3, m2, m0);
									const __m128i T0M2 = montgomery_sub_128(T0, MT2, m2, m0);
									const __m128i T1M3 = montgomery_mul_128(
										montgomery_sub_128(MT1, MT3, m2, m0), Imag, r, m1);
									_mm_storeu_si128((__m128i*)(a + j0),
										montgomery_add_128(T0P2, T1P3, m2, m0));
									_mm_storeu_si128((__m128i*)(a + j1),
										montgomery_sub_128(T0P2, T1P3, m2, m0));
									_mm_storeu_si128((__m128i*)(a + j2),
										montgomery_add_128(T0M2, T1M3, m2, m0));
									_mm_storeu_si128((__m128i*)(a + j3),
										montgomery_sub_128(T0M2, T1M3, m2, m0));
								}
							}
							xx = mul(xx, dw[ctz((jh += 4))]);
						}
					}
					else {
						const __m256i m0 = _mm256_set1_epi32(0);
						const __m256i m1 = _mm256_set1_epi32(mod);
						const __m256i m2 = _mm256_set1_epi32(mod + mod);
						const __m256i r = _mm256_set1_epi32(m_modPreset.R);
						const __m256i Imag = _mm256_set1_epi32(imag);
						uint32 ww = one, xx = one, wx = one;
						for (int jh = 0; jh < u;) {
							if (jh == 0) {
								int j0 = 0;
								int j1 = v;
								int j2 = j1 + v;
								int j3 = j2 + v;
								int je = v;
								for (; j0 < je; j0 += 8, j1 += 8, j2 += 8, j3 += 8) {
									const __m256i T0 = _mm256_loadu_si256((__m256i*)(a + j0));
									const __m256i T1 = _mm256_loadu_si256((__m256i*)(a + j1));
									const __m256i T2 = _mm256_loadu_si256((__m256i*)(a + j2));
									const __m256i T3 = _mm256_loadu_si256((__m256i*)(a + j3));
									const __m256i T0P2 = montgomery_add_256(T0, T2, m2, m0);
									const __m256i T1P3 = montgomery_add_256(T1, T3, m2, m0);
									const __m256i T0M2 = montgomery_sub_256(T0, T2, m2, m0);
									const __m256i T1M3 = montgomery_mul_256(
										montgomery_sub_256(T1, T3, m2, m0), Imag, r, m1);
									_mm256_storeu_si256((__m256i*)(a + j0),
										montgomery_add_256(T0P2, T1P3, m2, m0));
									_mm256_storeu_si256((__m256i*)(a + j1),
										montgomery_sub_256(T0P2, T1P3, m2, m0));
									_mm256_storeu_si256((__m256i*)(a + j2),
										montgomery_add_256(T0M2, T1M3, m2, m0));
									_mm256_storeu_si256((__m256i*)(a + j3),
										montgomery_sub_256(T0M2, T1M3, m2, m0));
								}
							}
							else {
								ww = mul(xx, xx), wx = mul(ww, xx);
								const __m256i WW = _mm256_set1_epi32(ww);
								const __m256i WX = _mm256_set1_epi32(wx);
								const __m256i XX = _mm256_set1_epi32(xx);
								int j0 = jh * v;
								int j1 = j0 + v;
								int j2 = j1 + v;
								int j3 = j2 + v;
								int je = j1;
								for (; j0 < je; j0 += 8, j1 += 8, j2 += 8, j3 += 8) {
									const __m256i T0 = _mm256_loadu_si256((__m256i*)(a + j0));
									const __m256i T1 = _mm256_loadu_si256((__m256i*)(a + j1));
									const __m256i T2 = _mm256_loadu_si256((__m256i*)(a + j2));
									const __m256i T3 = _mm256_loadu_si256((__m256i*)(a + j3));
									const __m256i MT1 = montgomery_mul_256(T1, XX, r, m1);
									const __m256i MT2 = montgomery_mul_256(T2, WW, r, m1);
									const __m256i MT3 = montgomery_mul_256(T3, WX, r, m1);
									const __m256i T0P2 = montgomery_add_256(T0, MT2, m2, m0);
									const __m256i T1P3 = montgomery_add_256(MT1, MT3, m2, m0);
									const __m256i T0M2 = montgomery_sub_256(T0, MT2, m2, m0);
									const __m256i T1M3 = montgomery_mul_256(
										montgomery_sub_256(MT1, MT3, m2, m0), Imag, r, m1);
									_mm256_storeu_si256((__m256i*)(a + j0),
										montgomery_add_256(T0P2, T1P3, m2, m0));
									_mm256_storeu_si256((__m256i*)(a + j1),
										montgomery_sub_256(T0P2, T1P3, m2, m0));
									_mm256_storeu_si256((__m256i*)(a + j2),
										montgomery_add_256(T0M2, T1M3, m2, m0));
									_mm256_storeu_si256((__m256i*)(a + j3),
										montgomery_sub_256(T0M2, T1M3, m2, m0));
								}
							}
							xx = mul(xx, dw[ctz((jh += 4))]);
						}
					}
					u <<= 2;
					v >>= 2;
				}
			}

			void intt(uint32* a, int n, int normalize = true) {
				int k = n ? ctz(n) : 0;
				if (k == 0) return;
				if (k == 1) {
					uint32 a1 = a[1];
					a[1] = sub(a[0], a[1]);
					a[0] = add(a[0], a1);
					if (normalize) {
						uint32 invn = inv(m_modPreset.IntoMontgomery(2));
						a[0] = mul(a[0], invn);
						a[1] = mul(a[1], invn);
					}
					return;
				}
				int u = 1 << (k - 2);
				int v = 1;
				uint32 one = m_modPreset.IntoMontgomery(1);
				uint32 imag = dy[1];
				while (u) {
					if (v == 1) {
						uint32 ww = one, xx = one, yy = one;
						u <<= 2;
						for (int jh = 0; jh < u;) {
							ww = mul(xx, xx), yy = mul(xx, imag);
							uint32 t0 = a[jh + 0], t1 = a[jh + 1];
							uint32 t2 = a[jh + 2], t3 = a[jh + 3];
							uint32 t0p1 = add(t0, t1), t2p3 = add(t2, t3);
							uint32 t0m1 = mul(sub(t0, t1), xx), t2m3 = mul(sub(t2, t3), yy);
							a[jh + 0] = add(t0p1, t2p3), a[jh + 2] = mul(sub(t0p1, t2p3), ww);
							a[jh + 1] = add(t0m1, t2m3), a[jh + 3] = mul(sub(t0m1, t2m3), ww);
							xx = mul(xx, dy[ctz(jh += 4)]);
						}
					}
					else if (v == 4) {
						const __m128i m0 = _mm_set1_epi32(0);
						const __m128i m1 = _mm_set1_epi32(mod);
						const __m128i m2 = _mm_set1_epi32(mod + mod);
						const __m128i r = _mm_set1_epi32(m_modPreset.R);
						const __m128i Imag = _mm_set1_epi32(imag);
						uint32 ww = one, xx = one, yy = one;
						u <<= 2;
						for (int jh = 0; jh < u;) {
							if (jh == 0) {
								int j0 = 0;
								int j1 = v;
								int j2 = v + v;
								int j3 = j2 + v;
								for (; j0 < v; j0 += 4, j1 += 4, j2 += 4, j3 += 4) {
									const __m128i T0 = _mm_loadu_si128((__m128i*)(a + j0));
									const __m128i T1 = _mm_loadu_si128((__m128i*)(a + j1));
									const __m128i T2 = _mm_loadu_si128((__m128i*)(a + j2));
									const __m128i T3 = _mm_loadu_si128((__m128i*)(a + j3));
									const __m128i T0P1 = montgomery_add_128(T0, T1, m2, m0);
									const __m128i T2P3 = montgomery_add_128(T2, T3, m2, m0);
									const __m128i T0M1 = montgomery_sub_128(T0, T1, m2, m0);
									const __m128i T2M3 = montgomery_mul_128(
										montgomery_sub_128(T2, T3, m2, m0), Imag, r, m1);
									_mm_storeu_si128((__m128i*)(a + j0),
										montgomery_add_128(T0P1, T2P3, m2, m0));
									_mm_storeu_si128((__m128i*)(a + j2),
										montgomery_sub_128(T0P1, T2P3, m2, m0));
									_mm_storeu_si128((__m128i*)(a + j1),
										montgomery_add_128(T0M1, T2M3, m2, m0));
									_mm_storeu_si128((__m128i*)(a + j3),
										montgomery_sub_128(T0M1, T2M3, m2, m0));
								}
							}
							else {
								ww = mul(xx, xx), yy = mul(xx, imag);
								const __m128i WW = _mm_set1_epi32(ww);
								const __m128i XX = _mm_set1_epi32(xx);
								const __m128i YY = _mm_set1_epi32(yy);
								int j0 = jh * v;
								int j1 = j0 + v;
								int j2 = j1 + v;
								int j3 = j2 + v;
								int je = j1;
								for (; j0 < je; j0 += 4, j1 += 4, j2 += 4, j3 += 4) {
									const __m128i T0 = _mm_loadu_si128((__m128i*)(a + j0));
									const __m128i T1 = _mm_loadu_si128((__m128i*)(a + j1));
									const __m128i T2 = _mm_loadu_si128((__m128i*)(a + j2));
									const __m128i T3 = _mm_loadu_si128((__m128i*)(a + j3));
									const __m128i T0P1 = montgomery_add_128(T0, T1, m2, m0);
									const __m128i T2P3 = montgomery_add_128(T2, T3, m2, m0);
									const __m128i T0M1 = montgomery_mul_128(
										montgomery_sub_128(T0, T1, m2, m0), XX, r, m1);
									const __m128i T2M3 = montgomery_mul_128(
										montgomery_sub_128(T2, T3, m2, m0), YY, r, m1);
									_mm_storeu_si128((__m128i*)(a + j0),
										montgomery_add_128(T0P1, T2P3, m2, m0));
									_mm_storeu_si128(
										(__m128i*)(a + j2),
										montgomery_mul_128(montgomery_sub_128(T0P1, T2P3, m2, m0), WW,
											r, m1));
									_mm_storeu_si128((__m128i*)(a + j1),
										montgomery_add_128(T0M1, T2M3, m2, m0));
									_mm_storeu_si128(
										(__m128i*)(a + j3),
										montgomery_mul_128(montgomery_sub_128(T0M1, T2M3, m2, m0), WW,
											r, m1));
								}
							}
							xx = mul(xx, dy[ctz(jh += 4)]);
						}
					}
					else {
						const __m256i m0 = _mm256_set1_epi32(0);
						const __m256i m1 = _mm256_set1_epi32(mod);
						const __m256i m2 = _mm256_set1_epi32(mod + mod);
						const __m256i r = _mm256_set1_epi32(m_modPreset.R);
						const __m256i Imag = _mm256_set1_epi32(imag);
						uint32 ww = one, xx = one, yy = one;
						u <<= 2;
						for (int jh = 0; jh < u;) {
							if (jh == 0) {
								int j0 = 0;
								int j1 = v;
								int j2 = v + v;
								int j3 = j2 + v;
								for (; j0 < v; j0 += 8, j1 += 8, j2 += 8, j3 += 8) {
									const __m256i T0 = _mm256_loadu_si256((__m256i*)(a + j0));
									const __m256i T1 = _mm256_loadu_si256((__m256i*)(a + j1));
									const __m256i T2 = _mm256_loadu_si256((__m256i*)(a + j2));
									const __m256i T3 = _mm256_loadu_si256((__m256i*)(a + j3));
									const __m256i T0P1 = montgomery_add_256(T0, T1, m2, m0);
									const __m256i T2P3 = montgomery_add_256(T2, T3, m2, m0);
									const __m256i T0M1 = montgomery_sub_256(T0, T1, m2, m0);
									const __m256i T2M3 = montgomery_mul_256(
										montgomery_sub_256(T2, T3, m2, m0), Imag, r, m1);
									_mm256_storeu_si256((__m256i*)(a + j0),
										montgomery_add_256(T0P1, T2P3, m2, m0));
									_mm256_storeu_si256((__m256i*)(a + j2),
										montgomery_sub_256(T0P1, T2P3, m2, m0));
									_mm256_storeu_si256((__m256i*)(a + j1),
										montgomery_add_256(T0M1, T2M3, m2, m0));
									_mm256_storeu_si256((__m256i*)(a + j3),
										montgomery_sub_256(T0M1, T2M3, m2, m0));
								}
							}
							else {
								ww = mul(xx, xx), yy = mul(xx, imag);
								const __m256i WW = _mm256_set1_epi32(ww);
								const __m256i XX = _mm256_set1_epi32(xx);
								const __m256i YY = _mm256_set1_epi32(yy);
								int j0 = jh * v;
								int j1 = j0 + v;
								int j2 = j1 + v;
								int j3 = j2 + v;
								int je = j1;
								for (; j0 < je; j0 += 8, j1 += 8, j2 += 8, j3 += 8) {
									const __m256i T0 = _mm256_loadu_si256((__m256i*)(a + j0));
									const __m256i T1 = _mm256_loadu_si256((__m256i*)(a + j1));
									const __m256i T2 = _mm256_loadu_si256((__m256i*)(a + j2));
									const __m256i T3 = _mm256_loadu_si256((__m256i*)(a + j3));
									const __m256i T0P1 = montgomery_add_256(T0, T1, m2, m0);
									const __m256i T2P3 = montgomery_add_256(T2, T3, m2, m0);
									const __m256i T0M1 = montgomery_mul_256(
										montgomery_sub_256(T0, T1, m2, m0), XX, r, m1);
									const __m256i T2M3 = montgomery_mul_256(
										montgomery_sub_256(T2, T3, m2, m0), YY, r, m1);
									_mm256_storeu_si256((__m256i*)(a + j0),
										montgomery_add_256(T0P1, T2P3, m2, m0));
									_mm256_storeu_si256(
										(__m256i*)(a + j2),
										montgomery_mul_256(montgomery_sub_256(T0P1, T2P3, m2, m0), WW,
											r, m1));
									_mm256_storeu_si256((__m256i*)(a + j1),
										montgomery_add_256(T0M1, T2M3, m2, m0));
									_mm256_storeu_si256(
										(__m256i*)(a + j3),
										montgomery_mul_256(montgomery_sub_256(T0M1, T2M3, m2, m0), WW,
											r, m1));
								}
							}
							xx = mul(xx, dy[ctz(jh += 4)]);
						}
					}
					u >>= 4;
					v <<= 2;
				}
				if (k & 1) {
					v = 1 << (k - 1);
					if (v < 8) {
						for (int j = 0; j < v; ++j) {
							uint32 ajv = sub(a[j], a[j + v]);
							a[j] = add(a[j], a[j + v]);
							a[j + v] = ajv;
						}
					}
					else {
						const __m256i m0 = _mm256_set1_epi32(0);
						const __m256i m2 = _mm256_set1_epi32(mod + mod);
						int j0 = 0;
						int j1 = v;
						for (; j0 < v; j0 += 8, j1 += 8) {
							const __m256i T0 = _mm256_loadu_si256((__m256i*)(a + j0));
							const __m256i T1 = _mm256_loadu_si256((__m256i*)(a + j1));
							__m256i naj = montgomery_add_256(T0, T1, m2, m0);
							__m256i najv = montgomery_sub_256(T0, T1, m2, m0);
							_mm256_storeu_si256((__m256i*)(a + j0), naj);
							_mm256_storeu_si256((__m256i*)(a + j1), najv);
						}
					}
				}
				if (normalize) {
					uint32 invn = inv(m_modPreset.IntoMontgomery(n));
					for (int i = 0; i < n; i++) a[i] = mul(a[i], invn);
				}
			}

			static std::map<uint32, std::unique_ptr<NTT>> instanceCache;
			static NTT* GetInstancePtr(const Modint::ModPreset& MOD) {
				if (instanceCache.count(MOD.MOD) == 0) {
					instanceCache.insert(std::make_pair(MOD.MOD, std::unique_ptr<NTT>(new NTT(MOD))));
				}
				return instanceCache.at(MOD.MOD).get();
			}

		};

		std::map<uint32, std::unique_ptr<NTT>> NTT::instanceCache;


	} // anonymous namespace






	uint32* ModintVector::getPtr() const { return m_mem.getPtr(); }

	ModintVector::ModintVector(uint32 MOD) :
		PRESET(Modint::ModPreset::MakeFromVariableMod(MOD)),
		m_mem(PRESET.NTT_LENGTH)
	{
		m_mem.malloc();
	}

	ModintVector::ModintVector(Modint::ModPreset MOD_PRESET) :
		PRESET(MOD_PRESET),
		m_mem(PRESET.NTT_LENGTH)
	{
		m_mem.malloc();
	}

	uint32 ModintVector::length() const { return m_mem.getLength(); }

	// すべての要素を 0 にする
	// 0 はモンゴメリ表現でも通常表現でも 0 になる
	void ModintVector::clear() const { m_mem.clear(); }

	// Array に読み込む
	Array<uint32> ModintVector::read(uint32 lengthMax) const {
		uint32 lengthToRead = std::min<uint32>(lengthMax, length());
		Array<uint32> res(lengthToRead);
		m_mem.copyTo(res);
		return res;
	}

	// Array から書き込む
	void ModintVector::write(const Array<uint32>& src) const {
		m_mem.copyFrom(src);
	}

	// すべての要素はモンゴメリ表現
	// r : モンゴメリ表現
	// A[i] <- r A[i]
	ModintVector& ModintVector::multiplyInMontgomery(uint32 r) {
		uint32* p = getPtr();
		uint32 len8 = length() / 8 * 8;
		__m256i multiplier = _mm256_set1_epi32(r);
		__m256i mmr = _mm256_set1_epi32(PRESET.R);
		__m256i mmmod = _mm256_set1_epi32(PRESET.MOD);
		for (uint32 i = 0; i < len8; i += 8) {
			__m256i pVal = _mm256_loadu_si256((__m256i*)(p + i));
			__m256i res = montgomery_mul_256(pVal, multiplier, mmr, mmmod);
			_mm256_storeu_si256((__m256i*)(p + i), res);
		}
		for (uint32 i = len8; i < length(); i++) {
			p[i] = PRESET.MontgomeryMultiply(p[i], r);
		}
		return *this;
	}

	// すべての要素はモンゴメリ表現
	// r : モンゴメリ表現
	// A[i] <- r[i] A[i]
	ModintVector& ModintVector::multiplyInMontgomery(const ModintVector& r) {
		if (length() != r.length()) throw Error(U"FpsNyaan::ModintVector::multiplyInMontgomery : length dont match");
		uint32* p = getPtr();
		uint32* q = r.getPtr();
		uint32 len8 = length() / 8 * 8;
		__m256i mmr = _mm256_set1_epi32(PRESET.R);
		__m256i mmmod = _mm256_set1_epi32(PRESET.MOD);
		for (uint32 i = 0; i < len8; i += 8) {
			__m256i pVal = _mm256_loadu_si256((__m256i*)(p + i));
			__m256i qVal = _mm256_loadu_si256((__m256i*)(q + i));
			__m256i res = montgomery_mul_256(pVal, qVal, mmr, mmmod);
			_mm256_storeu_si256((__m256i*)(p + i), res);
		}
		for (uint32 i = len8; i < length(); i++) {
			p[i] = PRESET.MontgomeryMultiply(p[i], q[i]);
		}
		return *this;
	}

	// 通常表現をモンゴメリ表現に変換
	ModintVector& ModintVector::intoMontgomery() {
		return multiplyInMontgomery(PRESET.R2);
	}

	// モンゴメリ・リダクション
	// モンゴメリ表現を通常表現に変換
	ModintVector& ModintVector::montgomeryReduction() {
		return multiplyInMontgomery(1);
	}

	// OmegaTable を用いてバタフライ演算を行う
	ModintVector& ModintVector::butterflyInMontgomery() {
		auto ntt = NTT::GetInstancePtr(PRESET);
		ntt->ntt(getPtr(), length());
		return *this;
	}

	// OmegaTable を用いてバタフライ演算の逆を行う
	ModintVector& ModintVector::butterflyInvInMontgomery() {
		auto ntt = NTT::GetInstancePtr(PRESET);
		ntt->intt(getPtr(), length(), false);
		multiplyInMontgomery(PRESET.MontgomeryInv(PRESET.IntoMontgomery(PRESET.NTT_LENGTH)));
		return *this;
	}



} // namespace nyaan_convolution
