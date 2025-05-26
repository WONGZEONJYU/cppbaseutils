#include "xthread_pool.hpp"
#include "xtask.hpp"
#include <iostream>

using namespace std;
using namespace this_thread;

namespace xtd {

	void XThreadPool::Init(const uint64_t &num){
		m_thread_num_ = num;
	}

	void XThreadPool::Start(){

		if (m_thread_num_ <= 0){
			cerr << "Please Init XThreadPool\n";
			return;
		}

		{
			unique_lock lock(m_mux_);
			if (!m_threads_.empty()){
				cerr << "Thread pool has start!\n";
				return;
			}
		}

		for (uint64_t i {}; i < m_thread_num_; ++i) {
			m_threads_.push_back(make_shared<thread>(&XThreadPool::Run, this));
		}
	}

	void XThreadPool::Stop(){
		m_is_exit_ = true;
		m_cv_.notify_all();
		for (const auto& th: m_threads_){
			th->join();
		}
	}

	void XThreadPool::Run(){
		while (!m_is_exit_){
			const auto &task{get_Task()};
			if (!task) {
				continue;
			}
			++task_run_count_;
			try{
				task->set_return_(task->Run());
			}catch (const exception& e){
				cerr << "error : " << e.what() << '\n';
			}
			--task_run_count_;
		}
	}

	void XThreadPool::add_Task(const std::shared_ptr<XTask> &task){
		task->set_exit_([this]{ return is_exit(); });
		{
			unique_lock lock(m_mux_);
			m_tasks_.push_back(task);
		}
		m_cv_.notify_one();
	}

	std::shared_ptr<XTask> XThreadPool::get_Task(){
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

		const auto task{ m_tasks_.front() };
		m_tasks_.pop_front();
		return task;
	}
}
