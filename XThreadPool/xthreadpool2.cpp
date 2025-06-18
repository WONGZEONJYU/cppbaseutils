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

using Tid_t = XSize_t;

class XThread_ final {
    enum class Private{};
    using task_t = std::function<void(const Tid_t &)>;
    task_t m_taskFunc_{};
public:
    X_DEFAULT_COPY_MOVE(XThread_)
    explicit XThread_(task_t &&t,Private):m_taskFunc_(std::move(t)){}
    ~XThread_() = default;

    void start() {
        std::thread(m_taskFunc_,get_id()).detach();
    }

    [[nodiscard]] Tid_t get_id()const{
        return reinterpret_cast<Tid_t>(std::addressof(m_taskFunc_));
    }

    using XThread_Ptr = std::shared_ptr<XThread_>;
    static XThread_Ptr create(auto &&t){
        try{
            return std::make_shared<XThread_>(std::forward<decltype(t)>(t),Private{});
        }catch (const std::exception &){
            std::cerr << "create Thread err!\n";
            return {};
        }
    }
};

class XThreadPool2::XThreadPool2Private final {
    X_DISABLE_COPY_MOVE(XThreadPool2Private)
    enum class Private{};
    static constexpr auto WAIT_MINUTES {60};
#if defined(__LP64__)
    static constexpr auto MAX_THREADS_SIZE{1024LL},
    MAX_TASKS_SIZE{INT64_MAX};
#else
    static constexpr auto
    MAX_THREADS_SIZE{1024L},MAX_TASKS_SIZE{INT32_MAX};
#endif
    std::deque<XAbstractTask2_Ptr> m_tasksQueue_{};
    std::unordered_map<Tid_t, XThread_::XThread_Ptr> m_threadsContainer_{};
    mutable std::recursive_mutex m_mtx_{};
    std::condition_variable_any m_taskQue_Cond_{},m_exit_Cond_{};
public:
    Mode m_mode{};
    XAtomicBool m_is_poolRunning{};
    XAtomicInteger<XSize_t> m_initThreadsSize{},m_idleThreadsSize{},m_busyThreadsSize{},
        m_threadsSizeThreshold{MAX_THREADS_SIZE},
        m_tasksSizeThreshold{MAX_TASKS_SIZE};

    explicit XThreadPool2Private(Private){}
    ~XThreadPool2Private() = default;
    static XThreadPool2Private_Ptr create(){
        try {
            return std::make_unique<XThreadPool2Private>(Private{});
        } catch (const std::exception &) {
            std::cerr << "XThreadPool2 data mem alloc failed\n";
            return {};
        }
    }

    XAbstractTask2_Ptr acquireTask(){

        const auto last_time{std::chrono::high_resolution_clock::now()};

        std::unique_lock lock(m_mtx_);

        while (m_tasksQueue_.empty()){

            if (!m_is_poolRunning.loadAcquire()){
                return {};
            }

            if (Mode::CACHE == m_mode){
                if (std::cv_status::timeout == m_taskQue_Cond_.wait_for(lock,std::chrono::seconds(1))){
                    if (const auto dur{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - last_time).count()};
                        dur >= WAIT_MINUTES){
                        std::cerr << " acquireTask timeout\n";
                        return {};
                    }else{
                        //std::cout << "no task duration: " << dur << " seconds\n" << std::flush;
                    }
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

    XAbstractTask2_Ptr taskJoin(const XAbstractTask2_Ptr& task){
        if (!task){
            std::cerr << __PRETTY_FUNCTION__ << " tips: task is empty!\n";
            return {};
        }

        std::unique_lock lock(m_mtx_);

        if(!m_taskQue_Cond_.wait_for(lock,std::chrono::seconds(1),[this]{
            return m_tasksQueue_.size() < m_tasksSizeThreshold.loadAcquire();
        })){
            task->set_result_({});
            std::cerr << "task queue is full, join task failed.\n";
            return task;
        }

        task->set_exit_function_([this]{return m_is_poolRunning.loadAcquire();});

        m_tasksQueue_.push_back(task);
        m_taskQue_Cond_.notify_all();

        if (Mode::CACHE == m_mode && m_is_poolRunning.loadAcquire()
            && m_threadsContainer_.size() < m_threadsSizeThreshold.loadAcquire()
            && m_tasksQueue_.size() > m_idleThreadsSize.loadAcquire()){

            if (const auto th{XThread_::create([this](const auto &id){run(id);})}){
                m_threadsContainer_[th->get_id()] = th;
                std::cout << "new Thread\n" << std::flush;
                m_idleThreadsSize.fetchAndAddOrdered(1);
                th->start();
            }
        }

        return task;
    }

    void start(const XSize_t &threadSize){

        if (m_is_poolRunning.loadAcquire()){
            return;
        }

        auto thSize{threadSize};

        std::cout << " Pool Start Running\n" << std::flush;

        if (thSize > m_threadsSizeThreshold.loadAcquire()){
            std::cerr << "threadSize Reached the upper limit\n";
            thSize = m_threadsSizeThreshold.loadAcquire();
        }
#if 1
        if (Mode::CACHE == m_mode && m_tasksQueue_.size() > thSize){
            auto diff{m_tasksQueue_.size() - thSize};
            if (diff > m_threadsSizeThreshold.loadAcquire()){
                diff = m_threadsSizeThreshold.loadAcquire();
            }
            thSize = static_cast<decltype(thSize)>(diff);
        }
#endif
        m_initThreadsSize.storeRelease(thSize);

        std::unique_lock lock(m_mtx_);

        for (decltype(thSize) i{}; i < thSize; ++i){
            if (auto th{XThread_::create([this](const auto &id){run(id);})}){
                m_threadsContainer_[th->get_id()] = std::move(th);
                m_idleThreadsSize.fetchAndAddRelease(1);
            }
        }

        m_is_poolRunning.storeRelease(true);

        for (const auto& item : m_threadsContainer_ | std::views::values){
            item->start();
        }
    }

    void stop(){
#if 0
        if (!m_is_poolRunning.loadAcquire()){
            for (const auto &item:m_tasksQueue_){
                item->set_result_({});
            }
        }
#endif
        m_is_poolRunning.storeRelease({});
        m_taskQue_Cond_.notify_all();
        std::unique_lock lock(m_mtx_);
        m_taskQue_Cond_.notify_all();
        m_exit_Cond_.wait(lock,[this]{
            return m_threadsContainer_.empty();
        });
    }

    void run(const XSize_t &threadId){
        while (true){
            if (const auto task{acquireTask()}){
                try{
                    m_idleThreadsSize.fetchAndSubRelease(1);
                    (*task)();
                    m_idleThreadsSize.fetchAndAddRelease(1);
                }catch(const std::exception &e){
                    std::cerr << __PRETTY_FUNCTION__ << " exception: " << e.what() << "\n" << std::flush;
                }
            }else{
                std::cerr << "threadId = " << threadId <<" end\n" << std::flush;
                break;
            }
        }

        {
            std::unique_lock lock(m_mtx_);
            m_threadsContainer_.erase(threadId);
        }
        m_idleThreadsSize.fetchAndSubRelease(1);
        m_exit_Cond_.notify_all();
    }

    auto currentThreadsSize() const{
        std::unique_lock lock(m_mtx_);
        return static_cast<XSize_t>(m_threadsContainer_.size());
    }

    auto currentTasksSize() const{
        std::unique_lock lock(m_mtx_);
        return static_cast<XSize_t>(m_tasksQueue_.size());
    }
};

XSize_t XThreadPool2::currentThreadsSize() const {
    X_D();
    return d->currentThreadsSize();
}

XSize_t XThreadPool2::idleThreadsSize() const {
    X_D();
    return d->m_idleThreadsSize.loadAcquire();
}

XSize_t XThreadPool2::busyThreadsSize() const {
    X_D();
    return 0;
}

XSize_t XThreadPool2::currentTasksSize() const {
    X_D();
    return d->currentTasksSize();
}

XThreadPool2::XThreadPool2(const Mode &mode,XThreadPool2Private_Ptr d_ptr):
m_d_(std::move(d_ptr)){
    m_d_->m_mode = mode;
}

XThreadPool2::~XThreadPool2(){
    stop();
}

void XThreadPool2::start(const XSize_t &threadSize){
    X_D();
    d->start(threadSize);
}

void XThreadPool2::stop(){
    X_D();
    d->stop();
}

void XThreadPool2::setMode(const Mode &mode){
    X_D();
    if (d->m_is_poolRunning.loadAcquire()){
        return;
    }
    d->m_mode = mode;
}

void XThreadPool2::setThreadsSizeThreshold(const XSize_t &n){
    X_D(XThreadPool2Private);
    if (d->m_is_poolRunning.loadAcquire()){
        std::cerr << "setThreadsSizeThreshold Must be in a stopped state\n" << std::flush;
        return;
    }
    d->m_threadsSizeThreshold.storeRelease(n);
}

void XThreadPool2::setTasksSizeThreshold(const XSize_t &n){
    X_D();
    if (d->m_is_poolRunning.loadAcquire()){
        std::cerr << "setTasksSizeThreshold Must be in a stopped state\n" << std::flush;
        return;
    }
    d->m_tasksSizeThreshold.storeRelease(n);
}

XAbstractTask2_Ptr XThreadPool2::taskJoin(const XAbstractTask2_Ptr& task){
    X_D();
    return d->taskJoin(task);
}

XThreadPool2_Ptr XThreadPool2::create(const Mode& mode){
    try{
        auto d_ptr{XThreadPool2Private::create()};
        if (!d_ptr){return {};}
        return std::make_shared<XThreadPool2>(mode,std::move(d_ptr));
    }catch (const std::exception &){
        return {};
    }
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
