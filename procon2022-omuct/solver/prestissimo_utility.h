#pragma once

#include "solver_interface.h"
#include "convolution_interface.h"

namespace Procon33 {
	namespace Prestissimo {


		static const int64 DIFF_COST_MULTIPLIER = 40;


		Array<String> GetPrecalcFileList(String dir, Array<String> tags);


		struct Reader {
			WaveInt32 wave;
			String fileName;
		};


		struct PreInfo {
			Array<Reader> readers;
		};

		struct ProblemAdditionalData {
			String problemId;
			int32 partIndex;
			WaveInt32 wave;
		};

		struct ReadDataPosition {
			int32 readId;
			int32 readOffset;
		};

		struct DiffScore {
			int32 readId = -1;
			int32 readSlide = -1;
			int64 score = INT64_MIN;
		};


		struct PrecalcForAReader {
			WaveInt32 wave;
			Array<int32> waveDiff;
			Procon33::ConvolutionInerfaceSimd fpsSourceReversed;
			Procon33::ConvolutionInerfaceSimd fpsSourceDiffReversed;
#ifdef ENABLE_CUDA_ALGORITHM
			FpsCuda::FpsInNTT fpsSourceReversedCuda;
			FpsCuda::FpsInNTT fpsSourceDiffReversedCuda;
#endif
		};

		struct PrecalcUnit {
			std::shared_ptr<const PreInfo> preInfo;
			Array<PrecalcForAReader> readers;
		};

		struct SolverStateRaw {
			std::shared_ptr<const PrecalcUnit> precalcUnit;
			Array<std::shared_ptr<const ProblemAdditionalData>> additionalData;
			std::map<String, std::shared_ptr<const ProblemInfo>> problemInfo;
			String solvingProblemId;
			int32 threadCount;
		};

		struct ChunkCluster {
			int32 leftIndex;
			int32 rightIndex;
			int32 offset; // cluster 間の位置関係
			int32 rangeOffset; // chunk の並びから cluster を取り出す位置
			int32 lengthSample;
		};

		struct SolverAnswerForProblem {
			Array<ReadDataPosition> readPos;
			Array<int32> chunkPos;
			Array<ChunkCluster> clusters;
			uint64 lossFunctionValue;
		};

		struct SolverFeedback {
			struct ReadDataInfo {
				int32 length;
			};
			std::map<String, Prestissimo::SolverAnswerForProblem> answer;
			std::map<String, double> progressBarsState;
			Array<ReadDataInfo> readDataInfo;
			uint64 lastRequestIndex;
		};


		class SolverState {
		public:
			void setAnswer(SolverAnswerForProblem);
			SolverAnswerForProblem getAnswer();
			void setToRun(bool on);
			bool isRunning();

			void setInturruption();
			bool getInturruption();

			void setProgressBarState(String key, double val);
			void deleteProgressBarState(String key);
			void deleteAllProgressBars();
			Array<std::pair<String, double>> getProgressBarsState();

			void setStateRaw(SolverStateRaw);
			std::shared_ptr<const SolverStateRaw> getStateRaw();

			void requestTerminate();
			bool getTerminateRequest();

		private:

			std::mutex m_mutex;

			SolverAnswerForProblem m_answer;
			bool m_isRunning = false;
			Optional<String> m_problemId;
			std::map<String, double> m_progressBarsState;

			std::shared_ptr<const SolverStateRaw> m_stateRaw;

			bool m_inturruption = false;
			bool m_terminate = false;
		};

	}
}

