#include "montgomery_modint_info.h"

#include <map>


namespace Modint {

	namespace {

		std::map<uint32, ModPreset> ModPresetCache;

	}


	ModPreset ModPreset::MakeFromVariableMod(uint32 MOD) {
		if (ModPresetCache.count(MOD) == 0) {
			const uint32 PRIM = PrimitiveRoot::GetVal(MOD);
			const uint32 NTTSZ = 19;
			ModPresetCache.emplace(MOD, ModPreset{
				MOD,
				MontgomeryInfo::GetR(MOD),
				MontgomeryInfo::GetR2(MOD),
				PRIM,
				NTTSZ,
				(uint32)1 << NTTSZ,
				MontgomeryInfo::PowMod(PRIM, MOD >> NTTSZ, MOD)
			});
		}
		return ModPresetCache.at(MOD);
	}

}

