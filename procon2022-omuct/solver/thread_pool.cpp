
#include "thread_pool.h"

#include <deque>
#include <mutex>
#include <thread>
#include <future>
#include <condition_variable>
#include <chrono>


namespace Procon33 {


	namespace {


		class ThreadPoolImpl;


		class ThreadPoolWorker {
		public:

			ThreadPoolWorker(ThreadPoolImpl* parent, uint32 index);

			~ThreadPoolWorker();

			// その時点で持っているタスクは完了するが、新しいタスクは受け取らない
			void terminate();

			void notifyAll();

			// その時点で持っているタスクは完了して同期し、その後新しいタスクを受け取る
			void sync();

		private:
			ThreadPoolImpl* const m_parent;
			bool m_hasTask; // タスク実行中かどうか
			bool m_terminationRequested; // スレッド終了の条件
			std::mutex m_thisMutex; // m_index 以外のメンバを保護
			const uint32 m_index;

			// - キューにタスクが追加されたとき
			// - 終了命令時
			// - タスク完了時
			std::condition_variable m_condition;

			std::thread m_thread;

			void threadTask(); // std::thread で実行されるメソッド
		};


		const bool QUEUE_TASK_AFTER_TERMINATION = true;


		class ThreadPoolImpl : public ThreadPool {
		public:

			ThreadPoolImpl(uint32 threadCount);

			ThreadPoolImpl(ThreadPoolImpl&&) = delete;
			ThreadPoolImpl(const ThreadPoolImpl&) = delete;
			~ThreadPoolImpl();

			// タスクをキューに追加する
			void pushTask(Task&& task);

			// 少なくとも今キューにあるタスクをすべて完了するか、終了信号まで待つ
			void sync();

			// キューにタスクがあれば 1 つ取り出す
			Optional<Task> pullTask();

			// それぞれのスレッドは、その時点で持っているタスクは完了するが、新しいタスクは受け取らない
			void terminate();

		private:

			std::deque<Task> m_taskQueue;
			bool m_terminationRequested; // スレッド終了の条件

			std::mutex m_thisMutex; // m_workers 以外のメンバを保護
			std::condition_variable m_condition;

			Array<std::unique_ptr<ThreadPoolWorker>> m_workers;

		};



		ThreadPoolWorker::ThreadPoolWorker(ThreadPoolImpl* parent, uint32 index) :
			m_parent(parent),
			m_hasTask(false),
			m_terminationRequested(false),
			m_index(index),
			m_thread([&]() -> void { threadTask(); })
		{
			m_condition.notify_all();
		}

		ThreadPoolWorker::~ThreadPoolWorker() {
			terminate();
			if (m_thread.joinable()) m_thread.join();
		}

		void ThreadPoolWorker::threadTask() {

			while (true) {
				Optional<ThreadPool::Task> task = m_parent->pullTask();

				/* 終了命令を受けて終了するか、タスクを獲得するまで待機 */
				if(!task.has_value()) {

					{
						std::unique_lock lock(m_thisMutex);
						m_hasTask = false;
					}

					m_condition.notify_all();
					std::unique_lock lock(m_thisMutex);
					m_condition.wait(
						lock,
						[&]() -> bool {
							task = m_parent->pullTask();
							if (m_terminationRequested) return true;
							return task.has_value();
						}
					);

					// 終了命令が来たらすぐに終了
					if (m_terminationRequested) {
						break;
					}

					m_hasTask = true;
				}
				// タスク開始
				task.value()(m_index);
			}

		}

		void ThreadPoolWorker::sync() {
			// 終了命令かタスク消化まで待つ
			bool result;
			do{
				m_condition.notify_all();
				std::unique_lock lock(m_thisMutex);
				auto pred = [&]() -> bool { return this->m_terminationRequested || !this->m_hasTask; };
				result = m_condition.wait_for(lock, std::chrono::seconds(3), pred);
			} while (!result);
			//auto pred = [&]() -> bool { return this->m_terminationRequested || !this->m_hasTask; };
			//m_condition.wait(lock, pred);
		}

		void ThreadPoolWorker::terminate() {
			{
				std::lock_guard lock(m_thisMutex);
				m_terminationRequested = true;
			}
			m_condition.notify_all(); // 終了命令を通知
		}

		void ThreadPoolWorker::notifyAll() {
			m_condition.notify_all();
		}


		ThreadPoolImpl::ThreadPoolImpl(uint32 threadCount) :
			m_terminationRequested(false)
		{
			// threadCount 個のスレッドを開始
			m_workers.resize(threadCount);
			for (uint32 idx = 0; idx < threadCount; idx++) {
				m_workers[idx] = std::make_unique<ThreadPoolWorker>(this, idx);
			}
		}

		ThreadPoolImpl::~ThreadPoolImpl() { sync(); terminate(); }

		void ThreadPoolImpl::pushTask(ThreadPool::Task&& task) {
			{
				std::lock_guard lock(m_thisMutex);
					// 定数式を if に入れると警告がでるが、回避方法がわからない
#pragma warning(push)
#pragma warning(disable : 4127)
				if (!QUEUE_TASK_AFTER_TERMINATION && m_terminationRequested) return;
#pragma warning(pop)
				m_taskQueue.push_back(std::move(task));
			}
			for (auto& worker : m_workers) worker->notifyAll();
		}

		void ThreadPoolImpl::sync() {
			{
				std::unique_lock lock(m_thisMutex);

				// 今たまっているタスクを消化
				auto pred = [&]() -> bool { return this->m_terminationRequested || this->m_taskQueue.empty(); };
				m_condition.wait(lock, pred);
			}

			for (auto& worker : m_workers) worker->sync(); // スレッドに残ったタスクを待つ
		}

		Optional<ThreadPool::Task> ThreadPoolImpl::pullTask() {
			Optional<ThreadPool::Task> res = none;
			{
				std::unique_lock<std::mutex> lock(m_thisMutex);

				if (!m_taskQueue.empty()) {
					res = std::move(m_taskQueue.front());
					m_taskQueue.pop_front();
				}
			}
			m_condition.notify_all();
			return res;
		}

		void ThreadPoolImpl::terminate() {
			{
				std::lock_guard lock(m_thisMutex);
				m_terminationRequested = true;
			}
			m_condition.notify_all(); // 終了命令を通知
			for (auto& worker : m_workers) worker->terminate(); // 終了命令を各スレッドに転送
		}

	}


	ThreadPool::ThreadPool() {
	}


	std::unique_ptr<ThreadPool> ThreadPool::Construct(uint32 threadCount) {
		return std::unique_ptr<ThreadPool>(new ThreadPoolImpl(threadCount));
	}

} // namespace Procon33

