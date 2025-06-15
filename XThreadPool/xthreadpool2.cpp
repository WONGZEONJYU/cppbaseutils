#include "xthreadpool2.hpp"
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <deque>
#include <functional>
#include <iostream>
#include <ranges>
#include <XAtomic/xatomic.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

#if defined(__LP64__)
using idtype_t = uint64_t;
#else
using idtype_t = uint32_t;
#endif

class XThread_ final : public std::enable_shared_from_this<XThread_>{
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
    using XThread_ptr = std::shared_ptr<XThread_>;
    static XThread_ptr create(task_t &&t){
        try{
            return std::make_shared<XThread_>(std::move(t),Private{});
        }catch (const std::exception &){
            std::cerr << "create Thread err!\n";
            return {};
        }
    }
};

class XThreadPool2::XThreadPool2Private final {
    enum class Private{};
#if defined(__LP64__)
    static constexpr inline auto MAX_TASKS_SIZE{UINT64_MAX},
            MAX_THREADS_SIZE{1024ULL};
#else
    static constexpr inline auto MAX_THREADS_SIZE{1024UL},MAX_TASKS_SIZE{UINT32_MAX};
#endif
public:
    std::mutex m_mtx_{};
    std::condition_variable_any m_taskQue_Cond_{},
        m_exit_Cond_{};
    std::unordered_map<idtype_t, XThread_::XThread_ptr> m_threadsContainer{};
    std::deque<XAbstractTask2_Ptr> m_tasksQueue_{};
    std::atomic_bool m_is_poolRunning_{};

#if defined(__LP64__)
    XAtomicInteger<uint64_t> m_initThreadsSize_{},
        m_idleThreadSize_{},
        m_threadsSizeThreshold_{MAX_THREADS_SIZE},
        m_tasksSizeThreshold_{MAX_TASKS_SIZE};
#else
    XAtomicInteger<uint32_t> m_initThreadsSize_{},
        m_idleThreadSize_{},
        m_threadsSizeThreshold_{},
        m_tasksSizeThreshold_{};
#endif

    explicit XThreadPool2Private(Private){}
    ~XThreadPool2Private() = default;
    static XThreadPool2Private_Ptr create(){
        try {
            return std::make_shared<XThreadPool2Private>(Private{});
        } catch (std::exception &) {
            std::cerr << "XThreadPool2 data mem alloc failed\n";
            return {};
        }
    };
};

[[maybe_unused]] uint64_t XThreadPool2::currentThreadsSize() {
    X_D(XThreadPool2Private);
    std::unique_lock lock{d->m_mtx_};
    return d->m_threadsContainer.size();
}

[[maybe_unused]] uint64_t XThreadPool2::idleThreadsSize() const {
    X_D(XThreadPool2Private);
    return d->m_idleThreadSize_.loadRelaxed();
}

XThreadPool2::XThreadPool2(const Mode& mode,Private):
m_d_(XThreadPool2Private::create()),
m_mode_(mode){}

XThreadPool2::~XThreadPool2(){
    stop();
}

[[maybe_unused]] void XThreadPool2::start(const uint64_t &threadSize){

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

[[maybe_unused]] void XThreadPool2::setMode(const Mode &mode){
    if (m_is_poolRunning_.load(std::memory_order_relaxed)){
        return;
    }
    m_mode_ = mode;
}

[[maybe_unused]] void XThreadPool2::setThreadsSizeThreshold(const uint64_t &n){
    if (m_is_poolRunning_.load(std::memory_order_relaxed)){
        std::cerr << "setThreadsSizeThreshold Must be in a stopped state\n" << std::flush;
        return;
    }
    m_threadsSizeThreshold_ = n;
}

[[maybe_unused]] void XThreadPool2::setTasksSizeThreshold(const uint64_t &n){
    if (m_is_poolRunning_.load(std::memory_order_relaxed)){
        std::cerr << "setTasksSizeThreshold Must be in a stopped state\n" << std::flush;
        return;
    }
    m_tasksSizeThreshold_ = n;
}

[[maybe_unused]] XAbstractTask2_Ptr XThreadPool2::joinTask(const XAbstractTask2_Ptr& task){

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

    task->set_exit_function_([this]{return m_is_poolRunning_.load(std::memory_order_relaxed);});

    m_tasksQueue_.push_back(task);
    m_taskQue_Cond_.notify_all();

    if (Mode::CACHE == m_mode_
        && m_threadsContainer.size() < m_threadsSizeThreshold_
        && m_tasksQueue_.size() > m_idleThreadSize_){

        if (const auto th{XThread_::create([this](const auto &id){run(id);})}){
            std::cout << "new Thread\n";
            m_threadsContainer[th->get_id()] = th;
            lock.unlock();
            th->start();
        }
    }

    return task;
}

[[maybe_unused]] XThreadPool2::XThreadPool2_Ptr XThreadPool2::create(const Mode& mode){
    try{
        return std::make_shared<XThreadPool2>(mode,Private{});
    }catch (const std::exception &){
        return {};
    }
}

void XThreadPool2::run(const idtype_t& threadId){
    //std::cout << __PRETTY_FUNCTION__ << " threadId = " << threadId <<" begin\n";
    while (m_is_poolRunning_){
        if (const auto task{acquireTask()}){
            --m_idleThreadSize_;
            task->exec_();
            ++m_idleThreadSize_;
        }else if (Mode::CACHE == m_mode_){
            std::cout << "timeout exit\n";
            break;
        }else{}
    }
    std::unique_lock lock(m_mtx_);
    m_threadsContainer.erase(threadId);
    m_exit_Cond_.notify_all();
    //std::cout << __PRETTY_FUNCTION__ << " threadId = " << threadId <<" end\n";
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
    m_tasksQueue_.pop_front();
    return task;
}

XTD_INLINE_NAMESPACE_END

XTD_NAMESPACE_END
