
#include "global.h"

#include "solver/solver_interface.h"
#include "solver_controller.h"
#include "match_manager_gui.h"

namespace Procon33 {

	static std::shared_ptr<Global> g;

	Global::Global()
		: solver(GetSolverInstance())
		, matchManager(std::make_shared<MatchServerManager>())
		, solverControl(std::make_shared<SolverController>(solver))
	{
	}

	std::shared_ptr<Global> Global::GetInstance() {
		if (!g) {
			g.reset(new Global());
		}
		return g;
	}

} // namespace Procon33
