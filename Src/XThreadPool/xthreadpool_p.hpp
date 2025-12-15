#ifndef XUTILS2_XTHREADPOOL_P_HPP
#define XUTILS2_XTHREAD_POOL_P_HPP 1

#include <XThreadPool/xthreadpool.hpp>
#include <deque>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <unordered_map>
#include <functional>
#include <XAtomic/xatomic.hpp>

#if __cplusplus >= 202002L
#include <ranges>
#endif

#ifdef UNUSE_STD_THREAD_LOCAL
#include "xthreadlocal.hpp"
#endif

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

static inline constexpr auto MAX_THREADS_SIZE{1024};
#if defined(__LP64__) || defined(_WIN64)
static inline constexpr auto MAX_TASKS_SIZE{INT64_MAX};
#else
static inline constexpr auto MAX_TASKS_SIZE{INT32_MAX};
#endif

static inline constexpr auto WAIT_MINUTES{60};

using Tid_t = xptrdiff;

class XThread_ final : public std::enable_shared_from_this<XThread_> {
    enum class Private{};
    using task_t = std::function<void(const Tid_t &)>;
    task_t m_taskFunc_{};

public:
    explicit XThread_(task_t && t,Private):m_taskFunc_(std::move(t)){}
    ~XThread_() = default;

    void start() const { std::thread(m_taskFunc_, reinterpret_cast<Tid_t>(this)).detach();}

    [[nodiscard]] auto get_id() const noexcept
    { return reinterpret_cast<Tid_t>(this); }

    using XThreadPtr = std::shared_ptr<XThread_>;

    template<typename F>
    static XThreadPtr create(F && t) noexcept
    { return makeShared<XThread_>(std::forward<decltype(t)>(t),Private{}); }

    X_DEFAULT_COPY_MOVE(XThread_)
};

class X_CLASS_EXPORT XThreadPoolPrivate final : public XThreadPoolData {

    X_DISABLE_COPY_MOVE(XThreadPoolPrivate)

public:
    X_DECLARE_PUBLIC(XThreadPool)
#ifdef UNUSE_STD_THREAD_LOCAL
    mutable XThreadLocalConstVoid m_isCurrentTask_{};
#endif

    std::deque<XAbstractRunnablePtr> m_tasksQueue{};
    std::unordered_map<Tid_t, XThread_::XThreadPtr> m_threadsContainer{};

    mutable std::recursive_mutex m_mtx{};
    mutable std::condition_variable_any m_taskQueueCond{},m_exitCond{};

    using Mode = XThreadPool::Mode;
    Mode m_mode{};
    XAtomicBool m_isPoolRunning{};
    XAtomicInteger<XSize_t> m_initThreadsSize{},m_idleThreadsSize{},m_busyThreadsSize{},
        m_threadTimeout{WAIT_MINUTES},
        m_threadsSizeThreshold{MAX_THREADS_SIZE},
        m_tasksSizeThreshold{MAX_TASKS_SIZE};

    constexpr XThreadPoolPrivate() = default;

    ~XThreadPoolPrivate() override = default;

    XAbstractRunnablePtr acquireTask();

    XAbstractRunnablePtr append( XAbstractRunnablePtr task);

    void start(XSize_t threadSize);

    void stop();

    void run(XSize_t threadId);

    auto currentThreadsSize() const noexcept
    { std::unique_lock lock(m_mtx); return static_cast<XSize_t>(m_threadsContainer.size()); }

    auto currentTasksSize() const noexcept
    { std::unique_lock lock(m_mtx); return static_cast<XSize_t>(m_tasksQueue.size()); }
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
