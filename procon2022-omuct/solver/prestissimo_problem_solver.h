#pragma once

#include "solver_interface.h"
#include "prestissimo_utility.h"

#include <mutex>

namespace Procon33 {
	namespace Prestissimo {


		class ProblemSolver {
		public:
			virtual ~ProblemSolver() = default;
			virtual void changePrecalc(std::shared_ptr<const PrecalcUnit> precalcUnit) = 0;
			virtual void giveAdditionalData(std::shared_ptr<const ProblemAdditionalData> additionalData) = 0;
			virtual void step() = 0;
			virtual AnswerRequestData getAnswer() = 0;
			virtual void draw(RectF globalRect) = 0;
		};


		std::shared_ptr<ProblemSolver> GetProblemSolverInstance(
			std::shared_ptr<const ProblemInfo> problemInfo,
			std::shared_ptr<SolverState> solverState,
			std::shared_ptr<Array<std::shared_ptr<class SolverSmall>>> threadWorkMemory
		);

	}
}
