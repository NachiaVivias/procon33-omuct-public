#include "chunk_idx_from_filename.h"

namespace Procon33 {

	int32 GetIndexFromChunkFileName(String x) {
		size_t p = 0;
		while (p < x.size() && (x[p] < '0' || x[p] > '9')) p++;
		if (p >= x.size()) return -1;
		int32 res = 0;
		while (p < x.size() && (x[p] >= '0' && x[p] <= '9')) {
			if (res >= 10000) return -1;
			res = res * 10 + (x[p++] - '0');
		}
		return res - 1;
	}

}
