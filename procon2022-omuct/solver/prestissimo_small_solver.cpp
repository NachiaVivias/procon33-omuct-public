#include "prestissimo_small_solver.h"


namespace Procon33 {

	namespace Prestissimo {


		void SolverSmall::minCost(
			std::shared_ptr<const PrecalcUnit> precalc,
			std::shared_ptr<const ProblemInfo> problemInfo,
			int k,
			Array<DiffScore>& ans,
			int targetLength,
			std::shared_ptr<const Procon33::ConvolutionInerfaceSimd> fpsDiff,
			std::shared_ptr<const Procon33::ConvolutionInerfaceSimd> fpsDiffDiff
		) {
			// 前計算チェック
			if (!(bool)precalc) throw Error(U"SolverSmall::convolutionMinCost : precalc is null");
			if (!(bool)problemInfo) throw Error(U"SolverSmall::convolutionMinCost : problemInfo is null");

			// 波形の差分 と 読み音声 の畳み込み
			m_fpsBuf.copyFrom(*fpsDiff);
			m_fpsBuf2.copyFrom(*fpsDiffDiff);

			m_fpsBuf.multiply(precalc->readers[k].fpsSourceReversed);
			m_res1 = m_fpsBuf.read();
			int res1Size = m_fpsBuf.size();
			m_fpsBuf2.multiply(precalc->readers[k].fpsSourceDiffReversed);
			m_res2 = m_fpsBuf2.read();
			int res2Size = m_fpsBuf2.size();

			m_sqSum.resize(precalc->readers[k].wave.length() + 1);

			// 2 乗の項を計算してスコア完成 (真値)
			{
				const int sourceLength = (int)precalc->readers[k].wave.length();

				m_sqSum[0] = 0;
				for (int t = 0; t < sourceLength; t++) {
					int64 h = precalc->readers[k].wave[sourceLength - 1 - t];
					m_sqSum[t + 1] = m_sqSum[t] - h * h;
				}
				for (int t = 0; t < res1Size; t++) {
					int ridx = std::min<int>(t + 1, sourceLength);
					int lidx = std::max<int>(t - targetLength + 1, 0);
					m_res1[t] = m_res1[t] * 2 + m_sqSum[ridx] - m_sqSum[lidx];
				}
			}

			// 2 乗の項を計算してスコア完成 (差分)
			{
				const int sourceLength = (int)precalc->readers[k].waveDiff.size();

				m_sqSum[0] = 0;
				for (int t = 0; t < sourceLength; t++) {
					int64 h = precalc->readers[k].waveDiff[sourceLength - 1 - t];
					m_sqSum[t + 1] = m_sqSum[t] - h * h;
				}
				for (int t = 0; t < res2Size; t++) {
					int ridx = std::min<int>(t + 1, sourceLength);
					int lidx = std::max<int>(t - targetLength + 1, 0);
					m_res2[t] = m_res2[t] * 2 + m_sqSum[ridx] - m_sqSum[lidx];
				}
			}

			for (int t = 0; t < res2Size; t++) m_res1[t + 1] += m_res2[t] * DIFF_COST_MULTIPLIER;

			DiffScore finalResult;
			for (int t = 1; t < res1Size; t++) {
				if (m_res1[t] > finalResult.score) {
					finalResult.readId = k;
					finalResult.readSlide = t - (int)precalc->readers[k].wave.length() + 1;
					finalResult.score = m_res1[t];
				}
			}
			ans = { finalResult };
		}


#ifdef ENABLE_CUDA_ALGORITHM
		void SolverSmall::minCostCuda(
			std::shared_ptr<const PrecalcUnit> precalc,
			std::shared_ptr<const ProblemInfo> problemInfo,
			int k,
			Array<DiffScore>& ans,
			int targetLength,
			std::shared_ptr<const FpsCuda::FpsInNTT> fpsDiff,
			std::shared_ptr<const FpsCuda::FpsInNTT> fpsDiffDiff
		) {
			// 前計算チェック
			if (!(bool)precalc) throw Error(U"SolverSmall::convolutionMinCost : precalc is null");
			if (!(bool)problemInfo) throw Error(U"SolverSmall::convolutionMinCost : problemInfo is null");

			// 波形の差分 と 読み音声 の畳み込み
			m_fpsBufCuda.copyFrom(*fpsDiff);
			m_fpsBufCuda2.copyFrom(*fpsDiffDiff);

			m_fpsBufCuda.multiply(precalc->readers[k].fpsSourceReversedCuda);
			m_fpsBufCuda.read(m_res1);
			int res1Size = m_fpsBufCuda.size();
			m_fpsBufCuda2.multiply(precalc->readers[k].fpsSourceDiffReversedCuda);
			m_fpsBufCuda2.read(m_res2);
			int res2Size = m_fpsBufCuda2.size();

			m_sqSum.resize(precalc->readers[k].wave.length() + 1);

			// 2 乗の項を計算してスコア完成 (真値)
			{
				const int sourceLength = (int)precalc->readers[k].wave.length();

				m_sqSum[0] = 0;
				for (int t = 0; t < sourceLength; t++) {
					int64 h = precalc->readers[k].wave[sourceLength - 1 - t];
					m_sqSum[t + 1] = m_sqSum[t] - h * h;
				}
				for (int t = 0; t < res1Size; t++) {
					int ridx = std::min<int>(t + 1, sourceLength);
					int lidx = std::max<int>(t - targetLength + 1, 0);
					m_res1[t] = m_res1[t] * 2 + m_sqSum[ridx] - m_sqSum[lidx];
				}
			}

			// 2 乗の項を計算してスコア完成 (差分)
			{
				const int sourceLength = (int)precalc->readers[k].waveDiff.size();

				m_sqSum[0] = 0;
				for (int t = 0; t < sourceLength; t++) {
					int64 h = precalc->readers[k].waveDiff[sourceLength - 1 - t];
					m_sqSum[t + 1] = m_sqSum[t] - h * h;
				}
				for (int t = 0; t < res2Size; t++) {
					int ridx = std::min<int>(t + 1, sourceLength);
					int lidx = std::max<int>(t - targetLength + 1, 0);
					m_res2[t] = m_res2[t] * 2 + m_sqSum[ridx] - m_sqSum[lidx];
				}
			}

			for (int t = 0; t < res2Size; t++) m_res1[t + 1] += m_res2[t] * DIFF_COST_MULTIPLIER;

			DiffScore finalResult;
			for (int t = 1; t < res2Size; t++) {
				if (m_res1[t] > finalResult.score) {
					finalResult.readId = k;
					finalResult.readSlide = t - (int)precalc->readers[k].wave.length() + 1;
					finalResult.score = m_res1[t];
				}
			}
			ans = { finalResult };
		}
#endif

	} // namespace Prestissimo

} // namespace Procon33
