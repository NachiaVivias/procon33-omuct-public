#pragma once

#include <Siv3D.hpp>



namespace Modint {


	struct PrimitiveRoot {
		static constexpr uint64 PowMod(uint32 MOD, uint64 a, uint64 i) {
			uint64 res = 1, aa = a;
			while (i) {
				if (i & 1) res = res * aa % MOD;
				aa = aa * aa % MOD;
				i /= 2;
			}
			return res;
		}
		static constexpr bool ExamineVal(uint32 MOD, uint32 g) {
			uint32 t = MOD - 1;
			for (uint32 d = 2; (uint64)d * d <= t; d++) if (t % d == 0) {
				if (PowMod(MOD, g, (MOD - 1) / d) == 1) return false;
				while (t % d == 0) t /= d;
			}
			if (t != 1) if (PowMod(MOD, g, (MOD - 1) / t) == 1) return false;
			return true;
		}
		static constexpr uint32 GetVal(uint32 MOD) {
			for (uint32 x = 2; x < MOD; x++) if (ExamineVal(MOD, x)) return x;
			return 0;
		}
	};


	class MontgomeryInfo {
	public:

		static constexpr uint32 GetR(uint32 mod) {
			uint32 ret = mod;
			ret *= (uint32)2 - mod * ret;
			ret *= (uint32)2 - mod * ret;
			ret *= (uint32)2 - mod * ret;
			ret *= (uint32)2 - mod * ret;
			return ret;
		}

		static constexpr uint32 GetR2(uint32 mod) { return ((((uint64)1 << 32) % mod) << 32) % mod; }

		static constexpr uint32 PowMod(uint32 a, uint32 i, uint32 mod) {
			if (i == 0) return 1;
			return (uint64)PowMod((uint64)a * a % mod, i / 2, mod) * ((i % 2) ? a : 1) % mod;
		}

		static constexpr uint32 InvModPrime(uint32 a, uint32 mod) { return PowMod(a, mod - 2, mod); }

	};



	struct ModPreset {
		const uint32 MOD;
		const uint32 R;
		const uint32 R2;
		const uint32 PRIMITIVE_ROOT;
		const uint32 NTT_SIZE;
		const uint32 NTT_LENGTH;
		const uint32 OMEGA;


		template<uint32 MOD>
		static ModPreset MakeFromConstMod() {
			constexpr uint32 PRIM = PrimitiveRoot::GetVal(MOD);
			constexpr uint32 NTTSZ = 19;
			return ModPreset{
				MOD,
				MontgomeryInfo::GetR(MOD),
				MontgomeryInfo::GetR2(MOD),
				PRIM,
				NTTSZ,
				1 << NTTSZ,
				MontgomeryInfo::PowMod(PRIM, (MOD - 1) >> NTTSZ, MOD)
			};
		}

		static ModPreset MakeFromVariableMod(uint32 MOD);

		constexpr uint32 MulHiWord(uint32 a, uint32 b) const {
			return ((uint64)a * b) >> 32;
		}

		constexpr uint32 ReduceIfGeqThanMod(uint32 x) const {
			return (x >= MOD) ? (x - MOD) : x;
		}

		constexpr uint32 MontgomeryReduction(uint32 a) const {
			return ReduceIfGeqThanMod(MOD - MulHiWord(a * R, MOD));
		}

		constexpr uint32 MontgomeryMultiply(uint32 a, uint32 b) const {
			return ReduceIfGeqThanMod(MulHiWord(a, b) + MOD - MulHiWord(a * b * R, MOD));
		}

		// a ** b
		constexpr uint32 MontgomeryPow(uint32 a, uint32 b) const {
			uint32 ans = MontgomeryMultiply(1, R2);
			uint32 aa = a;

			for (uint32 d = 0; d < 32; d++) {
				if (b & ((uint32)1 << d)) ans = MontgomeryMultiply(ans, aa);
				aa = MontgomeryMultiply(aa, aa);
			}
			return ans;
		}

		// 1 / a
		constexpr uint32 MontgomeryInv(uint32 a) const {
			return MontgomeryPow(a, MOD - 2);
		}

		// 1 / a
		constexpr uint32 IntoMontgomery(uint32 a) const {
			return MontgomeryMultiply(a, R2);
		}
	};

} // namespace Modint
