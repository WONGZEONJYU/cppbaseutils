#include "xthread_pool.hpp"
#include "xtask.hpp"
#include <iostream>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

using namespace std;
using namespace this_thread;

void XThreadPool::init(const uint64_t &num){
	if (num > thread::hardware_concurrency()) {
		cout << "The number of threads is greater than the number of cores\r\n";
		m_thread_num_ = thread::hardware_concurrency();
		return;
	}
	m_thread_num_ = num;
}

void XThreadPool::start(){

	if (m_thread_num_ <= 0){
		cerr << "Please Init XThreadPool\n";
		return;
	}

	unique_lock lock(m_mux_);
	if (!m_threads_.empty()){
		cerr << "Thread pool has start!\n";
		return;
	}

	m_is_exit_ = {};

	for (uint64_t i {}; i < m_thread_num_; ++i) {
		m_threads_.push_back(make_shared<thread>(&XThreadPool::Run,this));
	}
}

void XThreadPool::stop(){
	m_is_exit_ = true;
	m_cv_.notify_all();
	for (const auto& th: m_threads_){
		th->join();
	}
	m_threads_.clear();
}

void XThreadPool::Run(){
	while (!m_is_exit_){
		const auto &task{get_task()};
		if (!task) {
			continue;
		}
		++task_run_count_;
		try{
			task->set_return_(task->run());
		}catch (const exception& e){
			cerr << "error : " << e.what() << '\n';
		}
		--task_run_count_;
	}
}

void XThreadPool::add_task(const shared_ptr<XTask> &task){
	task->set_exit_cnd([this]{return m_is_exit_.load();});
	{
		unique_lock lock(m_mux_);
		m_tasks_.push_back(task);
	}
	m_cv_.notify_one();
}

void XThreadPool::remove_task(const std::shared_ptr<XTask> &task){
	{
		unique_lock lock(m_mux_);
		m_tasks_.remove(task);
	}
	m_cv_.notify_one();
}

shared_ptr<XTask> XThreadPool::get_task(){
	if (m_is_exit_) {
		return {};
	}

	unique_lock lock(m_mux_);

	if (m_tasks_.empty()){
		m_cv_.wait(lock);
	}

	if (m_tasks_.empty()) {
		return {};
	}

	const auto &task{m_tasks_.front()};
	m_tasks_.pop_front();
	return task;
}

XThreadPool::~XThreadPool(){
	stop();
}

XThreadPool_Ptr XThreadPool::create() {
	return make_shared<XThreadPool>(Private{});
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
