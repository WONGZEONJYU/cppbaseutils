#include "xthreadpool2.hpp"
#include <functional>
#include <iostream>
#include <ranges>

XTD_NAMESPACE_BEGIN
    XTD_INLINE_NAMESPACE_BEGIN(v1)

class XThreadPool2::XThread_ {
    static inline idtype_t thread_id{};
    idtype_t m_thread_id_{};
public:
    using task_t = std::function<void(const XThreadPool2::idtype_t &)>;
private:
    task_t m_taskFunc_{};
public:
    X_DEFAULT_COPY_MOVE(XThread_)
    explicit XThread_(task_t &&t):
    m_thread_id_(thread_id++),m_taskFunc_(std::move(t)){}
    ~XThread_() = default;
    void start() {
        std::thread (m_taskFunc_,m_thread_id_).detach();
    }

    [[nodiscard]] auto get_id()const{
        return m_thread_id_;
    }
};

XThreadPool2::XThreadPool2(const Mode& mode,Private):m_mode_(mode){}

XThreadPool2::~XThreadPool2(){
    stop();
}

void XThreadPool2::start(const uint64_t &threadSize){

    if (threadSize >= m_threadsSizeThreshold_ ){
        std::cerr << "threadSize Reached the upper limit\n";
        return;
    }

    if (m_is_poolRunning_){
        std::cerr << __PRETTY_FUNCTION__ << " Pool is Running,Start failed!\n";
        return;
    }

    m_initThreadsSize_ = threadSize;

    for (uint64_t i {}; i < threadSize; ++i){
        auto th {std::make_shared<XThread_>([this](const auto &id){
            run(id);
        })};
        m_threadsContainer[th->get_id()] = std::move(th);
    }

    m_is_poolRunning_ = true;

    for (const auto& item : m_threadsContainer | std::views::values){
        item->start();
        ++m_idleTaskSize_;
    }
}

void XThreadPool2::stop(){
    m_is_poolRunning_ = false;
    m_cond_.notify_all();
}

void XThreadPool2::joinTask(const XAbstractTask2_Ptr& task){

    if (!task){
        std::cerr << __PRETTY_FUNCTION__ << " tips: task is empty!\n";
        return;
    }

    std::unique_lock lock(m_mtx_);
    m_cond_.wait_for(lock,std::chrono::seconds(1),[this]{
        return m_tasksQueue_.size() < m_tasksSizeThreshold_;
    });



}

XThreadPool2::XThreadPool2_Ptr XThreadPool2::create(const Mode& mode){
    try{
        return std::make_shared<XThreadPool2>(mode,Private{});
    }catch (const std::exception &e){
        return {};
    }
}

void XThreadPool2::run(const idtype_t& threadId){
    while (m_is_poolRunning_){

        {
            std::unique_lock lock{m_mtx_};

        }

    }
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
