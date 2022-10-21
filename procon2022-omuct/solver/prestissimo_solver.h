#pragma once

#include "solver_interface.h"
#include "prestissimo_utility.h"

#include <mutex>

namespace Procon33 {
	namespace Prestissimo {


		class Solver : public SolverInterface {
		public:
			virtual ~Solver() = default;
		};


		std::shared_ptr<Solver> GetSolverInstance();

	}
}
