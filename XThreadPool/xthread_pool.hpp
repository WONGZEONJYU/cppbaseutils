#ifndef X_THREAD_POOL_HPP
#define X_THREAD_POOL_HPP

#include <thread>
#include <mutex>
#include <vector>
#include <list>
#include <atomic>
#include <condition_variable>
#include <memory>
#include "../XHelper/xhelper.hpp"

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XTask;
class XThreadPool;
using XThreadPool_Ptr = std::shared_ptr<XThreadPool>;

class [[maybe_unused]] XThreadPool final{

	std::shared_ptr<XTask> get_task();
	void Run();
	struct Private{explicit Private() = default;};
public:
	/*
	 * 线程池数量
	 */
	void init(const uint64_t &num = std::thread::hardware_concurrency());

	void start();

	void stop();

	void add_task(const std::shared_ptr<XTask> &);

	void remove_task(const std::shared_ptr<XTask> &);

	inline auto is_exit() const{ return m_is_exit_.load(); }

	inline auto task_run_count() const{ return task_run_count_.load(); }

	~XThreadPool();

private:
	std::mutex m_mux_{};
	using Threads_Ptr = std::shared_ptr<std::thread>;
	std::vector<Threads_Ptr> m_threads_{};
	std::list<std::shared_ptr<XTask> > m_tasks_{};
	std::condition_variable_any m_cv_{};
	std::atomic_uint64_t m_thread_num_{},task_run_count_ {};
	std::atomic_bool m_is_exit_{};
public:
	explicit XThreadPool(Private){}
	static XThreadPool_Ptr create();
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
