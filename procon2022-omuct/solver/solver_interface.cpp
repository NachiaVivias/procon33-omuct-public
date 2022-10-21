
#include "solver_interface.h"
#include "prestissimo_solver.h"
#include <mutex>
#include <map>

namespace Procon33 {

	const String SolverInterface::PROGRESS_ID_PRECALC = U"Solver Precalc";
	const String SolverInterface::PROGRESS_ID_STOP = U"Solver Stop";

	String SolverInterface::getLastErrorMessage() {
		return m_lastError;
	}

} // namespace Procon33


namespace Procon33 {

	std::shared_ptr<SolverInterface> GetSolverInstance() {
		return Prestissimo::GetSolverInstance();
	}

} // namespace Procon33

