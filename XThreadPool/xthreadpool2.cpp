#include "xthreadpool2.hpp"
#include <thread>
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
    mutable task_t m_taskFunc_{};
public:
    X_DEFAULT_COPY_MOVE(XThread_)
    explicit XThread_(task_t &&t,Private):m_taskFunc_(std::move(t)){}
    ~XThread_() = default;

    void start() const {
        std::thread(m_taskFunc_,get_id()).detach();
    }

    [[nodiscard]] Tid_t get_id() const {
        return reinterpret_cast<Tid_t>(std::addressof(m_taskFunc_));
    }

    using XThread_Ptr = std::shared_ptr<XThread_>;
    static XThread_Ptr create(auto &&t) {
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
    static constexpr auto WAIT_MINUTES{60};
#if defined(__LP64__)
    static constexpr auto MAX_THREADS_SIZE{1024LL},
    MAX_TASKS_SIZE{INT64_MAX};
#else
    static constexpr auto
    MAX_THREADS_SIZE{1024L},MAX_TASKS_SIZE{INT32_MAX};
#endif
    static inline thread_local void *sm_isCurrentTask_{};
    mutable std::deque<XAbstractTask2_Ptr> m_tasksQueue_{};
    mutable std::unordered_map<Tid_t, XThread_::XThread_Ptr> m_threadsContainer_{};
    mutable std::recursive_mutex m_mtx_{};
    mutable std::condition_variable_any m_taskQue_Cond_{},m_exit_Cond_{};
public:
    mutable Mode m_mode{};
    mutable XAtomicBool m_is_poolRunning{};
    mutable XAtomicInteger<XSize_t> m_initThreadsSize{},m_idleThreadsSize{},m_busyThreadsSize{},
        m_threadsSizeThreshold{MAX_THREADS_SIZE},
        m_tasksSizeThreshold{MAX_TASKS_SIZE};

    explicit XThreadPool2Private(Private){}

    ~XThreadPool2Private() = default;

    static XThreadPool2Private_Ptr create() {
        try {
            return std::make_unique<XThreadPool2Private>(Private{});
        } catch (const std::exception &) {
            std::cerr << "XThreadPool2 data mem alloc failed\n" << std::flush;
            return {};
        }
    }

    XAbstractTask2_Ptr acquireTask() const {

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
                    }
#if 0
                    else{
                        std::cout << "no task duration: " << dur << " seconds\n" << std::flush;
                    }
#endif
                }
            }else {
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

    XAbstractTask2_Ptr taskJoin(const XAbstractTask2_Ptr& task) const {

        if (!task){
            std::cerr << __PRETTY_FUNCTION__ << " tips: task is empty!\n" << std::flush;
            return task;
        }

        if (task.get() == sm_isCurrentTask_){
            std::cerr << __PRETTY_FUNCTION__ << " tips: Working Thread Call invalid\n" << std::flush;
            return task;
        }

        std::unique_lock lock(m_mtx_);

        if(!m_taskQue_Cond_.wait_for(lock,std::chrono::seconds(1),[this]{
            return m_tasksQueue_.size() < m_tasksSizeThreshold.loadAcquire();})){
            std::cerr << "task queue is full, join task failed.\n" << std::flush;
            return task;
        }

        task->set_exit_function_([this]{return m_is_poolRunning.loadAcquire();});
        task->set_occupy_();

        m_tasksQueue_.push_back(task);
        m_taskQue_Cond_.notify_all();

        if (CacheModel == m_mode &&
            m_is_poolRunning.loadAcquire() &&
            m_threadsContainer_.size() < m_threadsSizeThreshold.loadAcquire() &&
            m_tasksQueue_.size() > m_idleThreadsSize.loadAcquire()){
#if 0
            if (const auto th{XThread_::create([this](const auto &id){run(id);})}){
                m_threadsContainer_[th->get_id()] = th;
                std::cout << "new Thread\n" << std::flush;
                m_idleThreadsSize.fetchAndAddOrdered(1);
                th->start();
                m_taskQue_Cond_.notify_all();
            }
#else
            if (m_tasksQueue_.size() < m_threadsSizeThreshold.loadAcquire()){
                const auto thSize{static_cast<XSize_t>(m_tasksQueue_.size())};
                for (XSize_t i{};i < thSize;++i){
                    if (const auto th{XThread_::create([this](const auto &id){run(id);})}){
                        m_threadsContainer_[th->get_id()] = th;
                        std::cout << "new Thread\n" << std::flush;
                        m_idleThreadsSize.fetchAndAddOrdered(1);
                        th->start();
                        m_taskQue_Cond_.notify_all();
                    }
                }
            }
#endif
        }
        lock.unlock();
        m_taskQue_Cond_.notify_all();
        return task;
    }

    void start(const XSize_t &threadSize) const {

        if (m_is_poolRunning.loadAcquire()){
            return;
        }

        if (sm_isCurrentTask_){
            std::cerr << __PRETTY_FUNCTION__ << " tips: Working Thread Call invalid\n" << std::flush;
            return;
        }

        {
            std::unique_lock lock(m_mtx_);
            if(!m_threadsContainer_.empty() || m_idleThreadsSize.loadAcquire() > 0 || m_busyThreadsSize.loadAcquire() > 0){
                return;
            }
        }

        auto thSize{threadSize};

        std::cout << "Pool Start Running\n" << std::flush;

        if (thSize > m_threadsSizeThreshold.loadAcquire()){
            std::cerr << "threadsSize Reached the upper limit\n" << std::flush;
            thSize = m_threadsSizeThreshold.loadAcquire();
        }
#if 1
        if (CacheModel == m_mode){
            std::unique_lock lock(m_mtx_);
            if (const auto tasksSize{static_cast<decltype(thSize)>(m_tasksQueue_.size())};
                tasksSize > m_tasksSizeThreshold.loadAcquire()){
                thSize = m_tasksSizeThreshold.loadAcquire();
            }else{
                thSize = tasksSize > 0 ? tasksSize : thSize;
            }
        }
#endif
        m_initThreadsSize.storeRelease(0);

        std::unique_lock lock(m_mtx_);

        for (decltype(thSize) i{}; i < thSize; ++i){
            if (auto th{XThread_::create([this](const auto &id){run(id);})}){
                m_threadsContainer_[th->get_id()] = std::move(th);
                m_idleThreadsSize.fetchAndAddRelease(1);
                m_initThreadsSize.fetchAndAddRelease(1);
            }
        }

        m_is_poolRunning.storeRelease(true);

        for (const auto& item : m_threadsContainer_ | std::views::values){
            item->start();
        }
    }

    void stop() const {
        if (sm_isCurrentTask_){
            std::cerr << __PRETTY_FUNCTION__ << " tips: Working Thread Call invalid\n" << std::flush;
            return;
        }
        m_is_poolRunning.storeRelease({});
        m_taskQue_Cond_.notify_all();
        std::unique_lock lock(m_mtx_);
        m_taskQue_Cond_.notify_all();
        m_exit_Cond_.wait(lock,[this]{
            return m_threadsContainer_.empty();
        });
    }

    void run(const XSize_t &threadId) const {
        while (true){
            if (const auto task{acquireTask()}){
                const XRAII raii{[&]{
                    m_busyThreadsSize.fetchAndAddRelease(1);
                    m_idleThreadsSize.fetchAndSubRelease(1);
                    sm_isCurrentTask_ = task.get();
                },[this]{
                    m_idleThreadsSize.fetchAndAddRelease(1);
                    m_busyThreadsSize.fetchAndSubRelease(1);
                }};
                (*task)();
            }else{
                std::cerr << "threadId = " << std::this_thread::get_id() <<" end\n" << std::flush;
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

    auto currentThreadsSize() const {
        std::unique_lock lock(m_mtx_);
        return static_cast<XSize_t>(m_threadsContainer_.size());
    }

    auto currentTasksSize() const {
        std::unique_lock lock(m_mtx_);
        return static_cast<XSize_t>(m_tasksQueue_.size());
    }
};

unsigned XThreadPool2::cpuThreadsCount() {
    return std::thread::hardware_concurrency();
}

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
    return d->m_busyThreadsSize.loadAcquire();
}

XSize_t XThreadPool2::currentTasksSize() const {
    X_D();
    return d->currentTasksSize();
}

XThreadPool2::XThreadPool2(const Mode &mode,XThreadPool2Private_Ptr d_ptr):
m_d_(std::move(d_ptr)) {
    m_d_->m_mode = mode;
}

XThreadPool2::~XThreadPool2(){
    stop();
}

void XThreadPool2::start(const XSize_t &threadSize) {
    X_D();
    d->start(threadSize);
}

void XThreadPool2::stop() {
    X_D();
    d->stop();
}

bool XThreadPool2::isRunning() const {
    X_D();
    return d->m_is_poolRunning.loadAcquire();
}

void XThreadPool2::setMode(const Mode &mode) {
    X_D();
    if (d->m_is_poolRunning.loadAcquire()){
        return;
    }
    d->m_mode = mode;
}

XThreadPool2::Mode XThreadPool2::getMode() const {
    X_D();
    return d->m_mode;
}

void XThreadPool2::setThreadsSizeThreshold(const XSize_t &num) {
    X_D(XThreadPool2Private);
    if (d->m_is_poolRunning.loadAcquire()){
        std::cerr << "setThreadsSizeThreshold Must be in a stopped state\n" << std::flush;
        return;
    }
    d->m_threadsSizeThreshold.storeRelease(num);
}

XSize_t XThreadPool2::getThreadsSizeThreshold() const {
    X_D();
    return d->m_threadsSizeThreshold.loadAcquire();
}

void XThreadPool2::setTasksSizeThreshold(const XSize_t &num) {
    X_D();
    if (d->m_is_poolRunning.loadAcquire()){
        std::cerr << "setTasksSizeThreshold Must be in a stopped state\n" << std::flush;
        return;
    }
    d->m_tasksSizeThreshold.storeRelease(num);
}

XSize_t XThreadPool2::getTasksSizeThreshold() const {
    X_D();
    return d->m_tasksSizeThreshold.loadAcquire();
}

XAbstractTask2_Ptr XThreadPool2::taskJoin(const XAbstractTask2_Ptr& task) {
    X_D();
    const auto retTask{d->taskJoin(task)};
    start();
    return retTask;
}

XThreadPool2_Ptr XThreadPool2::create(const Mode& mode) {
    try{
        auto d_ptr{XThreadPool2Private::create()};
        if (!d_ptr){return {};}
        return std::make_shared<XThreadPool2>(mode,std::move(d_ptr));
    }catch (const std::exception &){
        return {};
    }
}

void sleep_for_ns(const XSize_t& ns) {
    std::this_thread::sleep_for(std::chrono::nanoseconds(ns));
}

void sleep_for_us(const XSize_t& us) {
    std::this_thread::sleep_for(std::chrono::microseconds(us));
}

void sleep_for_ms(const XSize_t& ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void sleep_for_s(const XSize_t& s) {
    std::this_thread::sleep_for(std::chrono::seconds(s));
}

void sleep_for_mins(const XSize_t& mins) {
    std::this_thread::sleep_for(std::chrono::minutes(mins));
}

void sleep_for_hours(const XSize_t& h) {
    std::this_thread::sleep_for(std::chrono::hours(h));
}

static auto now_(){
    return std::chrono::high_resolution_clock::now();
}

void sleep_until_ns(const XSize_t& ns) {
    std::this_thread::sleep_until(now_() + std::chrono::nanoseconds(ns));
}

void sleep_until_us(const XSize_t& us) {
    std::this_thread::sleep_until(now_() + std::chrono::microseconds(us));
}

void sleep_until_ms(const XSize_t& ms) {
    std::this_thread::sleep_until(now_() + std::chrono::milliseconds(ms));
}

void sleep_until_s(const XSize_t& s) {
    std::this_thread::sleep_until(now_() + std::chrono::seconds(s));
}

void sleep_until_mins(const XSize_t& mins) {
    std::this_thread::sleep_until(now_() + std::chrono::minutes(mins));
}

void sleep_until_hours(const XSize_t& h) {
    std::this_thread::sleep_until(now_() + std::chrono::hours(h));
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
