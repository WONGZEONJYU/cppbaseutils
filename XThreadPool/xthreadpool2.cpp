#include "xthreadpool2.hpp"
#include <condition_variable>
#include <mutex>
#include <unordered_map>
#include <deque>
#include <functional>
#include <iostream>
#include <ranges>
#include <XAtomic/xatomic.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

using Tid_t = Size_t;

class XThread_ final {
    static inline Tid_t thread_id{};
    enum class Private{};
public:
    using task_t = std::function<void(const Tid_t &)>;
private:
    Tid_t m_thread_id_{};
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

    using XThread_ptr = std::unique_ptr<XThread_>;
    static XThread_ptr create(task_t &&t){
        try{
            return std::make_unique<XThread_>(std::move(t),Private{});
        }catch (const std::exception &){
            std::cerr << "create Thread err!\n";
            return {};
        }
    }
};

class XThreadPool2::XThreadPool2Private final {
    X_DISABLE_COPY_MOVE(XThreadPool2Private)
    enum class Private{};
#if defined(__LP64__)
    static constexpr auto MAX_TASKS_SIZE{UINT64_MAX},
            MAX_THREADS_SIZE{1024ULL};
#else
    static constexpr auto
            MAX_THREADS_SIZE{1024UL},MAX_TASKS_SIZE{UINT32_MAX};
#endif
    std::deque<XAbstractTask2_Ptr> m_tasksQueue_{};
    std::unordered_map<Tid_t, XThread_::XThread_ptr> m_threadsContainer_{};
    mutable std::mutex m_mtx_{};
    std::condition_variable_any m_taskQue_Cond_{},m_exit_Cond_{};
public:
    Mode m_mode{};
    std::atomic_bool m_is_poolRunning{};
    XAtomicInteger<Size_t> m_initThreadsSize{},m_idleThreadSize{},
        m_threadsSizeThreshold{MAX_THREADS_SIZE},
        m_tasksSizeThreshold{MAX_TASKS_SIZE};

    explicit XThreadPool2Private(Private){}
    ~XThreadPool2Private() = default;
    static XThreadPool2Private_Ptr create(){
        try {
            return std::make_unique<XThreadPool2Private>(Private{});
        } catch (std::exception &) {
            std::cerr << "XThreadPool2 data mem alloc failed\n";
            return {};
        }
    }

    XAbstractTask2_Ptr acquireTask(){
        std::unique_lock lock(m_mtx_);
        while (m_tasksQueue_.empty()){
            if (!m_is_poolRunning.load(std::memory_order_acquire)){
                return {};
            }
            if (Mode::CACHE == m_mode){
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

    XAbstractTask2_Ptr joinTask(const XAbstractTask2_Ptr& task){
        if (!task){
            std::cerr << __PRETTY_FUNCTION__ << " tips: task is empty!\n";
            return {};
        }

        std::unique_lock lock(m_mtx_);

        if(!m_taskQue_Cond_.wait_for(lock,std::chrono::seconds(1),[this]{
            return m_tasksQueue_.size() < m_tasksSizeThreshold.loadAcquire();
        })){
            task->set_result_({});
            std::cerr << "task queue is full, join task fail." << std::endl;
            return task;
        }

        task->set_exit_function_([this]{return m_is_poolRunning.load(std::memory_order_relaxed);});

        m_tasksQueue_.push_back(task);
        m_taskQue_Cond_.notify_all();

        if (Mode::CACHE == m_mode
            && m_threadsContainer_.size() < m_threadsSizeThreshold
            && m_tasksQueue_.size() > m_idleThreadSize.loadAcquire()){
            if (auto th{XThread_::create([this](const auto &id){run(id);})}){
                std::cout << "new Thread\n";
                const auto p{th.get()};
                m_threadsContainer_[th->get_id()] = std::move(th);
                lock.unlock();
                m_idleThreadSize.fetchAndAddRelease(1);
                p->start();
            }
        }
        return task;
    }

    void start(const Size_t &threadSize){
        if (m_is_poolRunning.load(std::memory_order_acquire)){
            std::cerr << __PRETTY_FUNCTION__ << " Pool is Running,Start failed!\n";
            return;
        }
        if (threadSize >= m_threadsSizeThreshold.loadAcquire()){
            std::cerr << "threadSize Reached the upper limit\n";
            m_threadsSizeThreshold.storeRelease(std::thread::hardware_concurrency());
        }
        m_initThreadsSize.storeRelease(threadSize);
        for (uint64_t i {}; i < threadSize; ++i){
            if (auto th{XThread_::create([this](const auto &id){run(id);})}){
                m_threadsContainer_[th->get_id()] = std::move(th);
            }
        }
        m_is_poolRunning.store(true,std::memory_order_release);
        for (const auto& item : m_threadsContainer_ | std::views::values){
            item->start();
            m_idleThreadSize.fetchAndAddRelease(1);
        }
    }

    void stop(){
        m_is_poolRunning.store({},std::memory_order_release);
        std::unique_lock lock(m_mtx_);
        m_taskQue_Cond_.notify_all();
        m_exit_Cond_.wait(lock,[this]{
            return m_threadsContainer_.empty();
        });
    }

    void run(const uint64_t &threadId){
        //std::cout << __PRETTY_FUNCTION__ << " threadId = " << threadId <<" begin\n";
        while (m_is_poolRunning.load(std::memory_order_relaxed)){
            if (const auto task{acquireTask()}){
                m_idleThreadSize.fetchAndSubRelease(1);
                task->exec_();
                m_idleThreadSize.fetchAndAddRelease(1);
            }else if (Mode::CACHE == m_mode){
                std::cout << "timeout exit\n";
                break;
            }else{ }
        }
        {
            std::unique_lock lock(m_mtx_);
            m_threadsContainer_.erase(threadId);
        }
        m_exit_Cond_.notify_all();
        m_idleThreadSize.fetchAndSubRelease(1);
        //std::cout << __PRETTY_FUNCTION__ << " threadId = " << threadId <<" end\n";
    }

    auto currentThreadsSize() const{
        std::unique_lock lock(m_mtx_);
        return m_threadsContainer_.size();
    }
};

[[maybe_unused]] uint64_t XThreadPool2::currentThreadsSize() const{
    X_D(XThreadPool2Private);
    return d->currentThreadsSize();
}

[[maybe_unused]] uint64_t XThreadPool2::idleThreadsSize() const {
    X_D();
    return d->m_idleThreadSize.loadAcquire();
}

XThreadPool2::XThreadPool2(const Mode &mode,Private):
m_d_(XThreadPool2Private::create()){
    m_d_->m_mode = mode;
}

XThreadPool2::~XThreadPool2(){
    stop();
}

[[maybe_unused]] void XThreadPool2::start(const Size_t &threadSize){
    X_D(XThreadPool2Private);
    d->start(threadSize);
}

void XThreadPool2::stop(){
    X_D(XThreadPool2Private);
    d->stop();
}

[[maybe_unused]] void XThreadPool2::setMode(const Mode &mode){
    X_D(XThreadPool2Private);
    if (d->m_is_poolRunning.load(std::memory_order_acquire)){
        return;
    }
    d->m_mode = mode;
}

[[maybe_unused]] void XThreadPool2::setThreadsSizeThreshold(const uint64_t &n){
    X_D(XThreadPool2Private);
    if (d->m_is_poolRunning.load(std::memory_order_acquire)){
        std::cerr << "setThreadsSizeThreshold Must be in a stopped state\n" << std::flush;
        return;
    }
    d->m_threadsSizeThreshold.storeRelease(n);
}

[[maybe_unused]] void XThreadPool2::setTasksSizeThreshold(const uint64_t &n){
    X_D(XThreadPool2Private);
    if (d->m_is_poolRunning.load(std::memory_order_acquire)){
        std::cerr << "setTasksSizeThreshold Must be in a stopped state\n" << std::flush;
        return;
    }
    d->m_tasksSizeThreshold.storeRelease(n);
}

[[maybe_unused]] XAbstractTask2_Ptr XThreadPool2::joinTask(const XAbstractTask2_Ptr& task){
    X_D(XThreadPool2Private);
    return d->joinTask(task);
}

[[maybe_unused]] XThreadPool2::XThreadPool2_Ptr XThreadPool2::create(const Mode& mode){
    try{
        return std::make_shared<XThreadPool2>(mode,Private{});
    }catch (const std::exception &){
        return {};
    }
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
