#include "prestissimo_utility.h"


namespace Procon33 {

	namespace Prestissimo {

		Array<String> GetPrecalcFileList(String dir, Array<String> tags) {
			Array<String> filePaths = FileSystem::DirectoryContents(dir).filter(
				[&](const String& x)->bool {
					if (!x.ends_with(U".wav")) return false;
					String fileName = FileSystem::FileName(x);
					for (const String& key : tags) if (fileName.starts_with(key)) return true;
					return false;
				}
			);
			return filePaths;
		}

		void SolverState::setAnswer(SolverAnswerForProblem a) {
			std::lock_guard lock(m_mutex);
			m_answer = a;
		}
		SolverAnswerForProblem SolverState::getAnswer() {
			std::lock_guard lock(m_mutex);
			return m_answer;
		}
		void SolverState::setToRun(bool on) {
			std::lock_guard lock(m_mutex);
			m_isRunning = on;
		}
		bool SolverState::isRunning() {
			std::lock_guard lock(m_mutex);
			return m_isRunning;
		}

		void SolverState::setInturruption() {
			std::lock_guard lock(m_mutex);
			m_inturruption = true;
		}
		bool SolverState::getInturruption() {
			std::lock_guard lock(m_mutex);
			bool res = false; std::swap(res, m_inturruption);
			return res;
		}

		void SolverState::setProgressBarState(String key, double val) {
			std::lock_guard lock(m_mutex);
			m_progressBarsState[key] = val;
		}
		void SolverState::deleteProgressBarState(String key) {
			std::lock_guard lock(m_mutex);
			m_progressBarsState.erase(key);
		}
		void SolverState::deleteAllProgressBars() {
			std::lock_guard lock(m_mutex);
			m_progressBarsState.clear();
		}
		Array<std::pair<String, double>> SolverState::getProgressBarsState() {
			std::lock_guard lock(m_mutex);
			auto res = Array<std::pair<String, double>>(m_progressBarsState.begin(), m_progressBarsState.end());
			return res;
		}

		void SolverState::setStateRaw(SolverStateRaw a) {
			std::shared_ptr<const SolverStateRaw> res = std::make_shared<const SolverStateRaw>(std::move(a));
			std::lock_guard lock(m_mutex);
			std::swap(m_stateRaw, res);
		}
		std::shared_ptr<const SolverStateRaw> SolverState::getStateRaw() {
			std::lock_guard lock(m_mutex);
			return m_stateRaw;
		}

		void SolverState::requestTerminate() {
			std::lock_guard lock(m_mutex);
			m_terminate = true;
		}
		bool SolverState::getTerminateRequest() {
			std::lock_guard lock(m_mutex);
			return m_terminate;
		}

	} // namespace Prestissimo

} // namespace Procon33
