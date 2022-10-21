#pragma once

#include "../stdafx.h"
#include "convolution_interface.h"
#include "prestissimo_utility.h"


namespace Procon33 {

	namespace Prestissimo {



		class SolverSmall {
		public:

			Procon33::ConvolutionInerfaceSimd m_fpsBuf;
			Procon33::ConvolutionInerfaceSimd m_fpsBuf2;
			Array<int64> m_sqSum;
			Array<int64> m_res1;
			Array<int64> m_res2;

			void minCost(
				std::shared_ptr<const PrecalcUnit> precalc,
				std::shared_ptr<const ProblemInfo> problemInfo,
				int k,
				Array<DiffScore>& ans,
				int targetLength,
				std::shared_ptr<const Procon33::ConvolutionInerfaceSimd> fpsDiff,
				std::shared_ptr<const Procon33::ConvolutionInerfaceSimd> fpsDiffDiff
			);

#ifdef ENABLE_CUDA_ALGORITHM
			FpsCuda::FpsInNTT m_fpsBufCuda;
			FpsCuda::FpsInNTT m_fpsBufCuda2;

			void minCostCuda(
				std::shared_ptr<const PrecalcUnit> precalc,
				std::shared_ptr<const ProblemInfo> problemInfo,
				int k,
				Array<DiffScore>& ans,
				int targetLength,
				std::shared_ptr<const FpsCuda::FpsInNTT> fpsDiff,
				std::shared_ptr<const FpsCuda::FpsInNTT> fpsDiffDiff
			);
#endif
		};

	} // namespace Prestissimo

} // namespace Procon33
