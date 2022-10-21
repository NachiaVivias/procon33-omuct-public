#pragma once
#include "stdafx.h"

namespace Procon33 {

	struct Global {

		const std::shared_ptr<class SolverInterface> solver;
		const std::shared_ptr<class SolverController> solverControl;
		const std::shared_ptr<class MatchServerManager> matchManager;

		static std::shared_ptr<Global> GetInstance();

	private:
		Global();
	};

} // namespace Procon33
