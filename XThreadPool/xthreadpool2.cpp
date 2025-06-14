#include "xthreadpool2.hpp"
#include <functional>
#include <iostream>
#include <ranges>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XThreadPool2::XThread_ final{
    static inline idtype_t thread_id{};
    enum class Private{};
public:
    using task_t = std::function<void(const idtype_t &)>;
private:
    idtype_t m_thread_id_{};
    task_t m_taskFunc_{};
public:
    X_DEFAULT_COPY_MOVE(XThread_)
    explicit XThread_(task_t &&t,Private):
    m_thread_id_(thread_id++),m_taskFunc_(std::move(t)){}
    ~XThread_() = default;

    void start() {
        std::thread (m_taskFunc_,m_thread_id_).detach();
    }

    [[nodiscard]] auto get_id()const{
        return m_thread_id_;
    }

    static XThread_ptr create(task_t &&t){
        try{
            return std::make_shared<XThread_>(std::move(t),Private{});
        }catch (const std::exception &){
            std::cerr << "create Thread err!\n";
            return {};
        }
    }
};

uint64_t XThreadPool2::currentThreadsSize(){
    std::unique_lock lock{m_mtx_};
    return m_threadsContainer.size();
}

XThreadPool2::XThreadPool2(const Mode& mode,Private):m_mode_(mode){}

XThreadPool2::~XThreadPool2(){
    stop();
}

void XThreadPool2::start(const uint64_t &threadSize){

    if (m_is_poolRunning_){
        std::cerr << __PRETTY_FUNCTION__ << " Pool is Running,Start failed!\n";
        return;
    }

    if (threadSize >= m_threadsSizeThreshold_ ){
        std::cerr << "threadSize Reached the upper limit\n";
        return;
    }

    m_initThreadsSize_ = threadSize;

    for (uint64_t i {}; i < threadSize; ++i){
        if (auto th{XThread_::create([this](const auto &id){run(id);})}){
            m_threadsContainer[th->get_id()] = std::move(th);
        }
    }

    m_is_poolRunning_ = true;

    for (const auto& item : m_threadsContainer | std::views::values){
        item->start();
        ++m_idleThreadSize_;
    }
}

void XThreadPool2::stop(){
    m_is_poolRunning_.store({},std::memory_order_relaxed);
    std::unique_lock lock(m_mtx_);
    m_taskQue_Cond_.notify_all();
    m_exit_Cond_.wait(lock,[this]{return m_threadsContainer.empty();});
}

XAbstractTask2_Ptr XThreadPool2::joinTask(const XAbstractTask2_Ptr& task){

    if (!task){
        std::cerr << __PRETTY_FUNCTION__ << " tips: task is empty!\n";
        return {};
    }

    std::unique_lock lock(m_mtx_);

    if(!m_taskQue_Cond_.wait_for(lock,std::chrono::seconds(1),[this]{
        return m_tasksQueue_.size() < m_tasksSizeThreshold_;
    })){
        task->set_result_({});
        std::cerr << "task queue is full, join task fail." << std::endl;
        return task;
    }

    m_tasksQueue_.push(task);
    m_taskQue_Cond_.notify_all();

    if (Mode::CACHE == m_mode_
        && m_threadsContainer.size() < m_threadsSizeThreshold_
        && m_tasksQueue_.size() > m_idleThreadSize_){

        if (const auto th{XThread_::create([this](const auto &id){run(id);})}){
            m_threadsContainer[th->get_id()] = th;
            lock.unlock();
            th->start();
        }
    }

    return task;
}

XThreadPool2::XThreadPool2_Ptr XThreadPool2::create(const Mode& mode){
    try{
        return std::make_shared<XThreadPool2>(mode,Private{});
    }catch (const std::exception &){
        return {};
    }
}

void XThreadPool2::run(const idtype_t& threadId){
    while (m_is_poolRunning_){
        if (const auto task{acquireTask()}){
            --m_idleThreadSize_;
            task->exec_();
            ++m_idleThreadSize_;
        }else if (Mode::CACHE == m_mode_){
            break;
        }else{}
    }
    std::unique_lock lock(m_mtx_);
    m_threadsContainer.erase(threadId);
    m_exit_Cond_.notify_all();
}

XAbstractTask2_Ptr XThreadPool2::acquireTask() {

    std::unique_lock lock(m_mtx_);

    while (m_tasksQueue_.empty()){
        if (!m_is_poolRunning_){
            return {};
        }
        if (Mode::CACHE == m_mode_){
            if (std::cv_status::timeout == m_taskQue_Cond_.wait_for(lock,std::chrono::minutes(1))){
                return {};
            }
        }else{
            m_taskQue_Cond_.wait(lock);
        }
    }

    if (!m_tasksQueue_.empty()){
        m_taskQue_Cond_.notify_all();
    }

    const auto task{m_tasksQueue_.front()};
    m_tasksQueue_.pop();
    return task;
}

XTD_INLINE_NAMESPACE_END

XTD_NAMESPACE_END
