#pragma once
#include "solver_interface_queue.h"


namespace Procon33 {

	namespace SolverInternal {

		SolverInterfaceQueue::SolverInterfaceQueue() {
		}

		void SolverInterfaceQueue::request(std::shared_ptr<RequestPreCalc> req) {
			if (!(bool)req) throw Error(U"SolverInterface でエラー： req == nullptr");
			auto lock = std::lock_guard(m_mutex);
			m_precalc = req;
			m_list.push_back(req);
		}

		void SolverInterfaceQueue::request(std::shared_ptr<RequestAddProblem> req) {
			if (!(bool)req) throw Error(U"SolverInterface でエラー： req == nullptr");
			auto lock = std::lock_guard(m_mutex);
			m_problems[req->problemInfo.problemId] = req;
			m_list.push_back(req);
		}

		void SolverInterfaceQueue::request(std::shared_ptr<RequestSetProblem> req) {
			if (!(bool)req) throw Error(U"SolverInterface でエラー： req == nullptr");
			auto lock = std::lock_guard(m_mutex);
			if (m_problems.count(req->problemId) == 0) {
				throw Error(U"SolverInterface でエラー： 登録されていない問題を指定しようとしました。");
			}
			m_problemId = req;
			m_list.push_back(req);
		}

		void SolverInterfaceQueue::request(std::shared_ptr<RequestAddChunk> req) {
			if (!(bool)req) throw Error(U"SolverInterface でエラー： req == nullptr");
			auto lock = std::lock_guard(m_mutex);
			if (m_problems.count(req->problemId) == 0) {
				throw Error(U"SolverInterface でエラー： 登録されていない問題に断片を追加しようとしました。");
			}
			if (req->chunkIndex < 0) {
				throw Error(U"SolverInterface でエラー： 0 未満の断片番号が指定されました。");
			}
			if(m_problems.at(req->problemId)->problemInfo.chunksNum <= req->chunkIndex) {
				throw Error(U"SolverInterface でエラー： 断片数 n に対して n 以上の断片番号が指定されました。");
			}
			m_chunks[std::make_pair(req->problemId, req->chunkIndex)] = req;
			m_list.push_back(req);
		}

		void SolverInterfaceQueue::request(std::shared_ptr<RequestChangeThreadCount> req) {
			if (!(bool)req) throw Error(U"SolverInterface でエラー： req == nullptr");
			auto lock = std::lock_guard(m_mutex);
			if (req->threadCount <= 0) {
				throw Error(U"SolverInterface でエラー： threadCount に 0 以下の値が指定されました。");
			}
			m_threadCount = req->threadCount;
			m_list.push_back(req);
		}

		void SolverInterfaceQueue::request(std::shared_ptr<RequestRun> req) {
			if (!(bool)req) throw Error(U"SolverInterface でエラー： req == nullptr");
			auto lock = std::lock_guard(m_mutex);
			if (req->run && !(bool)m_precalc) {
				throw Error(U"SolverInterface でエラー： 前計算の前に起動しようとしました。");
			}
			if (req->run && !(bool)m_problemId) {
				throw Error(U"SolverInterface でエラー： 問題を設定する前に起動しようとしました。");
			}
			m_running = req->run;
			m_list.push_back(req);
		}

		AnyRequestWrap SolverInterfaceQueue::seek() {
			auto lock = std::lock_guard(m_mutex);
			if (m_list.empty()) return AnyRequestWrap::GetNull();
			return m_list.front();
		}

		AnyRequestWrap SolverInterfaceQueue::pop() {
			auto lock = std::lock_guard(m_mutex);
			if(m_list.empty()) return AnyRequestWrap::GetNull();
			auto res = m_list.front();
			m_list.pop_front();
			return res;
		}

		SolverInterfaceState SolverInterfaceQueue::getState() {
			auto lock = std::lock_guard(m_mutex);
			SolverInterfaceState res;
			res.precalc = m_precalc;
			using ProblemMapElemTy = std::pair<String, std::shared_ptr<RequestAddProblem>>;
			res.problems = Array<ProblemMapElemTy>(m_problems.begin(), m_problems.end()).map([](ProblemMapElemTy x) { return x.second; });
			using ChunkMapElemTy = std::pair<std::pair<String, int32>, std::shared_ptr<RequestAddChunk>>;
			res.chunks = Array<ChunkMapElemTy>(m_chunks.begin(), m_chunks.end()).map([](ChunkMapElemTy x) { return x.second; });
			res.problemId = m_problemId;
			res.running = m_running;
			res.threadCount = m_threadCount;
			return res;
		}

	}

} // namespace Procon33
