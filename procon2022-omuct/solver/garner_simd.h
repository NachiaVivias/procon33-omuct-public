#pragma once

#include "fps_nyaan_modify.h"


namespace Procon33{


	class GarnerSimd {
	public:

		GarnerSimd(Modint::ModPreset MOD0, Modint::ModPreset MOD1);

		FpsNyaan::AlignedArray<int64> solve(
			const FpsNyaan::ModintVector& a0,
			const FpsNyaan::ModintVector& a1,
			Optional<uint32> n = unspecified
		);

	private:

		const Modint::ModPreset m_mod0;
		const Modint::ModPreset m_mod1;
		const uint32 m_invMod0InMod1Montgomery;
		const uint32 m_invMod1InMod0Montgomery;

	};



} // namespace Procon33
