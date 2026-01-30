#include <utility>
#include "xthreadpool_p.hpp"
#include <iostream>
#include <XHelper/xspace.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

#ifndef UNUSE_STD_THREAD_LOCAL
    static constinit thread_local void * sm_isCurrentTask_{};
#endif

XAbstractRunnablePtr XThreadPoolPrivate::acquireTask() {

    using namespace std::chrono;

    auto const last_time { high_resolution_clock::now() };

    std::unique_lock lock(m_mtx);

    while (m_tasksQueue.empty()){

        if (!m_isPoolRunning.loadAcquire()){ return {}; }

        if (Mode::CACHE == m_mode) {
            using std::chrono::operator""s;
            if (std::cv_status::timeout == m_taskQueueCond.wait_for(lock,1s)) {
                if (auto const dur{duration_cast<seconds>(high_resolution_clock::now() - last_time)};
                    dur >= seconds (m_threadTimeout.loadAcquire()))
                {
                    std::cerr << "acquireTask timeout: " << m_threadTimeout.loadAcquire() << "\n" << std::flush;
                    return {};
                }
#if 0
                else { std::cout << "no task duration: " << dur << " seconds\n" << std::flush; }
#endif
            }
        } else { m_taskQueueCond.wait(lock); }
    }

    if (!m_tasksQueue.empty()) { m_taskQueueCond.notify_all(); }

    auto task{m_tasksQueue.front()};
    m_tasksQueue.pop_front();
    return task;
}

XAbstractRunnablePtr XThreadPoolPrivate::append(XAbstractRunnablePtr task) {

    if (!task){
        std::cerr << FUNC_SIGNATURE << " tips: task is empty!\n" << std::flush;
        return task;
    }

#ifdef UNUSE_STD_THREAD_LOCAL
    if (m_isCurrentTask_().value_or(nullptr) == task.get()){
#else
    if (task.get() == sm_isCurrentTask_){
#endif
        std::cerr << FUNC_SIGNATURE << " tips:Do not add your own behavior to the execution of your own thread functions\n" << std::flush;
        return task;
    }

    if (const void * old_value{};
        !task->Owner_().testAndSetOrdered({},this,old_value)){
        if (this != old_value){
            std::cerr << FUNC_SIGNATURE << " tips: This task has been added to the pool and cannot be added to other pools until it is completed\n";
            return task;
        }
    }

    std::unique_lock lock(m_mtx);

    using std::chrono::operator""s;
    if(!m_taskQueueCond.wait_for(lock,1s,[this]{
        return m_tasksQueue.size() < static_cast<decltype(m_tasksQueue.size())>(m_tasksSizeThreshold.loadAcquire());})){
        std::cerr << "task queue is full, join task failed.\n" << std::flush;
        return task;
    }

    task->set_exit_function_([this]{return m_isPoolRunning.loadAcquire();});
    task->resetRecall_();
    task->allow_get_();

    m_tasksQueue.push_back(task);
    m_taskQueueCond.notify_all();

    if (Mode::CACHE == m_mode &&
        m_isPoolRunning.loadAcquire() &&
        m_threadsContainer.size() < static_cast<decltype(m_threadsContainer.size())>(m_threadsSizeThreshold.loadAcquire()) &&
        m_tasksQueue.size() > static_cast<decltype(m_tasksQueue.size())>(m_idleThreadsSize.loadAcquire()))
    {

        auto thSize{static_cast<XSize_t>(m_tasksQueue.size())};

        if (thSize >= m_threadsSizeThreshold.loadAcquire()){
            thSize = m_threadsSizeThreshold.loadAcquire() - static_cast<decltype(thSize)>(m_threadsContainer.size());
        }

        for (decltype(thSize) i{};i < thSize;++i){
            if (const auto th{XThread_::create([this](const auto &id){run(id);})}){
                m_threadsContainer[th->get_id()] = th;
                m_idleThreadsSize.fetchAndAddRelease(1);
                th->start();
                m_taskQueueCond.notify_all();
            }
        }
        std::cout << "new add ThreadSize: " << thSize << "\n" << std::flush;
    }

    lock.unlock();
    m_taskQueueCond.notify_all();
    return task;
}

void XThreadPoolPrivate::start(XSize_t const threadSize) {

    if (m_isPoolRunning.loadAcquire()) { return; }

    {
        std::unique_lock lock(m_mtx);
        if(!m_threadsContainer.empty() || m_idleThreadsSize.loadAcquire() > 0 || m_busyThreadsSize.loadAcquire() > 0)
        { return; }
    }

    auto thSize{threadSize};

    if (thSize > m_threadsSizeThreshold.loadAcquire()) {
        std::cerr << "threadsSize Reached the upper limit\n" << std::flush;
        thSize = m_threadsSizeThreshold.loadAcquire();
    }

    if (Mode::CACHE == m_mode){
        std::unique_lock lock(m_mtx);
        if (const auto tasksSize{static_cast<decltype(thSize)>(m_tasksQueue.size())};
            tasksSize > m_tasksSizeThreshold.loadAcquire())
        { thSize = m_tasksSizeThreshold.loadAcquire(); }
        else {thSize = tasksSize > 0 ? tasksSize : thSize;}
    }

    std::cout << "Pool Start Running\n" << std::flush;
    std::cout << "Thread Initial quantity: " << thSize << "\n" << std::flush;

    m_initThreadsSize.storeRelease(0);

    std::unique_lock lock(m_mtx);

    for (decltype(thSize) i{}; i < thSize; ++i){
        if (auto th{ XThread_::create([this](const auto &id){run(id);}) }){
            m_threadsContainer[th->get_id()].swap(th);
            m_idleThreadsSize.fetchAndAddRelease(1);
            m_initThreadsSize.fetchAndAddRelease(1);
        }
    }

    m_isPoolRunning.storeRelease(true);

    for (auto const & item : m_threadsContainer | std::views::values)
    { item->start(); }
}

void XThreadPoolPrivate::stop() {
#ifndef UNUSE_STD_THREAD_LOCAL
    if (sm_isCurrentTask_){
#else
    if(m_isCurrentTask_()){
#endif
        std::cerr << FUNC_SIGNATURE << " tips: Working Thread Call invalid\n" << std::flush;
        return;
    }
    m_isPoolRunning.storeRelease({});
    m_taskQueueCond.notify_all();
    std::unique_lock lock(m_mtx);
    m_taskQueueCond.notify_all();
    m_exitCond.wait(lock,[this]()noexcept{ return m_threadsContainer.empty(); });
}

void XThreadPoolPrivate::run(XSize_t const threadId) {
    while (true){
        if (const auto task{acquireTask()}){
            XSpace const raii{[&]{
                m_busyThreadsSize.fetchAndAddRelease(1);
                m_idleThreadsSize.fetchAndSubRelease(1);
            },[this]{
                m_idleThreadsSize.fetchAndAddRelease(1);
                m_busyThreadsSize.fetchAndSubRelease(1);
            }};
#ifndef UNUSE_STD_THREAD_LOCAL
            sm_isCurrentTask_ = task.get();
#else
            const XThreadLocalStorageConstVoid set(m_isCurrentTask_,task.get());
#endif
            task->call();
        }else{
            std::cerr << "threadId = " << std::this_thread::get_id() <<" end\n" << std::flush;
            break;
        }
    }

    {
        std::unique_lock lock(m_mtx);
        m_threadsContainer.erase(threadId);
    }
    m_idleThreadsSize.fetchAndSubRelease(1);
    m_exitCond.notify_all();
}

unsigned XThreadPool::cpuThreadsCount()
{ return std::thread::hardware_concurrency(); }

XSize_t XThreadPool::currentThreadsSize() const noexcept
{ return d_func()->currentThreadsSize(); }

XSize_t XThreadPool::idleThreadsSize() const noexcept
{ return d_func()->m_idleThreadsSize.loadAcquire(); }

XSize_t XThreadPool::busyThreadsSize() const noexcept
{ return d_func()->m_busyThreadsSize.loadAcquire(); }

XSize_t XThreadPool::currentTasksSize() const noexcept
{ return d_func()->currentTasksSize(); }

XThreadPool::~XThreadPool()
{ stop(); }

void XThreadPool::start(XSize_t const threadSize)
{ d_func()->start(threadSize); }

void XThreadPool::stop()
{ d_func()->stop(); }

[[maybe_unused]] bool XThreadPool::isRunning() const noexcept
{ return d_func()->m_isPoolRunning.loadAcquire(); }

[[maybe_unused]] void XThreadPool::setMode(Mode const mode) noexcept {
    X_D(XThreadPool);
    if (d->m_isPoolRunning.loadAcquire()){ return; }
    d->m_mode = mode;
}

[[maybe_unused]] XThreadPool::Mode XThreadPool::getMode() const noexcept
{ return d_func()->m_mode; }

[[maybe_unused]] void XThreadPool::setThreadsSizeThreshold(XSize_t const num) noexcept {
    X_D(XThreadPool);
    if (d->m_isPoolRunning.loadAcquire()){
        std::cerr << "setThreadsSizeThreshold Must be in a stopped state\n" << std::flush;
        return;
    }
    d->m_threadsSizeThreshold.storeRelease(num);
}

[[maybe_unused]] XSize_t XThreadPool::getThreadsSizeThreshold() const noexcept {
    X_D(const XThreadPool);
    return d->m_threadsSizeThreshold.loadAcquire();
}

[[maybe_unused]] void XThreadPool::setTasksSizeThreshold(XSize_t const num) noexcept {
    X_D(XThreadPool);
    if (d->m_isPoolRunning.loadAcquire()){
        std::cerr << "setTasksSizeThreshold Must be in a stopped state\n" << std::flush;
        return;
    }
    d->m_tasksSizeThreshold.storeRelease(num);
}

[[maybe_unused]] XSize_t XThreadPool::getTasksSizeThreshold() const noexcept
{ return d_func()->m_tasksSizeThreshold.loadAcquire(); }

XAbstractRunnablePtr XThreadPool::appendHelper(XAbstractRunnablePtr task) {
    auto retTask{d_func()->append(std::move(task))};
    start();
    return retTask;
}

[[maybe_unused]] void XThreadPool::setThreadTimeout(XSize_t const seconds) noexcept {
    X_D(XThreadPool);
    if (d->m_isPoolRunning.loadAcquire()){
        std::cerr << "setThreadTimeout Must be in a stopped state\n" << std::flush;
        return;
    }
    if (seconds > 0){
        d->m_threadTimeout.storeRelease(seconds);
    } else{
        std::cerr << "setThreadTimeout Cannot be negative,set failed!\n" << std::flush;
    }
}

XThreadPoolPtr XThreadPool::create(Mode const mode) noexcept {
    auto ret{ CreateSharedPtr() };
    CHECK_EMPTY(ret);
    ret->setMode(mode);
    return ret;
}

XThreadPool::XThreadPool() = default;

bool XThreadPool::construct_() {
    auto dd{ makeUnique<XThreadPoolPrivate>() };
    CHECK_EMPTY(dd);
    m_d_ptr_ = std::move(dd);
    return true;
}

[[maybe_unused]] void sleep_for_ns(XSize_t const ns)
{ std::this_thread::sleep_for(std::chrono::nanoseconds(ns)); }

[[maybe_unused]] void sleep_for_us(XSize_t const us)
{ std::this_thread::sleep_for(std::chrono::microseconds(us)); }

[[maybe_unused]] void sleep_for_ms(XSize_t const ms)
{ std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

[[maybe_unused]] void sleep_for_s(XSize_t const s)
{ std::this_thread::sleep_for(std::chrono::seconds(s)); }

[[maybe_unused]] void sleep_for_mins(XSize_t const mins)
{ std::this_thread::sleep_for(std::chrono::minutes(mins)); }

[[maybe_unused]] void sleep_for_hours(XSize_t const h)
{ std::this_thread::sleep_for(std::chrono::hours(h)); }

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
