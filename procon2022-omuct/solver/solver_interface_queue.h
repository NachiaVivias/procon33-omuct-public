#pragma once
#include "solver_interface.h"
#include <list>
#include <variant>
#include <mutex>
#include <map>



namespace Procon33 {

	namespace SolverInternal {

		struct RequestPreCalc {
			uint64 requestIndex;
			String targetDir;
			Array<String> targetTags;
		};
		struct RequestAddProblem {
			uint64 requestIndex;
			ProblemInfo problemInfo;
		};
		struct RequestSetProblem {
			uint64 requestIndex;
			String problemId;
		};
		struct RequestAddChunk {
			uint64 requestIndex;
			String problemId;
			int32 chunkIndex;
			WaveInt32 wave;
		};
		struct RequestChangeThreadCount {
			uint64 requestIndex;
			int32 threadCount;
		};
		struct RequestRun {
			uint64 requestIndex;
			bool run;
		};

		using AnyRequest = std::variant<
			std::monostate,
			std::shared_ptr<RequestPreCalc>,
			std::shared_ptr<RequestAddProblem>,
			std::shared_ptr<RequestSetProblem>,
			std::shared_ptr<RequestAddChunk>,
			std::shared_ptr<RequestChangeThreadCount>,
			std::shared_ptr<RequestRun>
		>;

		struct AnyRequestWrap {
			AnyRequestWrap(AnyRequest x) : m(x) {}
			template<class TheType>
			bool isTheType() const { return std::holds_alternative<TheType>(m); }
			bool isNull() const { return isTheType<std::monostate>(); }
			static AnyRequest GetNull() { return AnyRequest(std::in_place_type_t<std::monostate>()); }
			AnyRequest get() const { return m; }
		private:
			AnyRequest m = GetNull();
		};


		struct SolverInterfaceState {
			std::shared_ptr<RequestPreCalc> precalc;
			Array<std::shared_ptr<RequestAddProblem>> problems;
			Array<std::shared_ptr<RequestAddChunk>> chunks;
			std::shared_ptr<RequestSetProblem> problemId;
			int32 threadCount;
			bool running;
		};


		class SolverInterfaceQueue {

			using QueueIteratorTy = std::list<AnyRequest>::iterator;

			std::mutex m_mutex;
			std::list<AnyRequest> m_list;
			std::shared_ptr<RequestPreCalc> m_precalc;
			std::map<String, std::shared_ptr<RequestAddProblem>> m_problems;
			std::map<std::pair<String, int32>, std::shared_ptr<RequestAddChunk>> m_chunks;
			std::shared_ptr<RequestSetProblem> m_problemId;
			int32 m_threadCount = 1;
			bool m_running = false;

		public:

			SolverInterfaceQueue();

			void request(std::shared_ptr<RequestPreCalc>);
			void request(std::shared_ptr<RequestAddProblem>);
			void request(std::shared_ptr<RequestSetProblem>);
			void request(std::shared_ptr<RequestAddChunk>);
			void request(std::shared_ptr<RequestChangeThreadCount>);
			void request(std::shared_ptr<RequestRun>);

			AnyRequestWrap seek();
			AnyRequestWrap pop();

			SolverInterfaceState getState();

		};

	}

} // namespace Procon33
