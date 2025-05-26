#ifndef X_THREAD_POOL_HPP
#define X_THREAD_POOL_HPP

#include <thread>
#include <mutex>
#include <vector>
#include <list>
#include <atomic>
#include <condition_variable>
#include <memory>

namespace xtd {
	class XTask;

	class XThreadPool {
		std::shared_ptr<XTask> get_Task();
		void Run();
	public:
		XThreadPool(const XThreadPool&) = delete;
		XThreadPool& operator=(const XThreadPool&) = delete;

		explicit XThreadPool() = default;

		/*
		 * 线程池数量
		 */
		void Init(const uint64_t &num = std::thread::hardware_concurrency());

		void Start();

		void Stop();

		void add_Task(const std::shared_ptr<XTask> &);

		inline auto is_exit() const{ return m_is_exit_.load(); }

		inline auto task_run_count() const{ return task_run_count_.load(); }

		virtual ~XThreadPool() { Stop(); }

	private:
		std::mutex m_mux_{};
		using Threads_Ptr = std::shared_ptr<std::thread>;
		std::vector<Threads_Ptr> m_threads_{};
		std::list<std::shared_ptr<XTask> > m_tasks_{};
		std::condition_variable_any m_cv_{};
		std::atomic_uint64_t m_thread_num_{},task_run_count_ {};
		std::atomic_bool m_is_exit_{};
	};
}

#endif
