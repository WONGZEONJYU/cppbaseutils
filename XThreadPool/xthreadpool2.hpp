#ifndef X_THREADPOOL2_HPP
#define X_THREADPOOL2_HPP

#include <XHelper/xhelper.hpp>
#include <atomic>
#include <queue>
#include <condition_variable>
#include <memory>
#include <unordered_map>
#include <thread>
#include "xabstracttask2.hpp"

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XThreadPool2 final {
    enum class Private{};
    class XThread_;
#if defined(__LP64__)
    static constexpr uint64_t MAX_THREADS_SIZE{1024},
    MAX_TASKS_SIZE{UINT64_MAX};
    using idtype_t = uint64_t;
#else
    static constexpr uint32_t MAX_THREADS_SIZE{UINT32_MAX};
    using idtype_t = uint32_t;
#endif
public:
    enum class Mode {
        FIXED,CACHE
    };
private:
    Mode m_mode_{};
    std::mutex m_mtx_{};
    std::condition_variable_any m_cond_{};
    std::condition_variable_any m_exit_cond_{};
    using XThread_ptr = std::shared_ptr<XThread_>;
    std::unordered_map<idtype_t, XThread_ptr> m_threadsContainer{};
    std::queue<XAbstractTask2_Ptr> m_tasksQueue_{};
    std::atomic_bool m_is_poolRunning_{};
#if defined(__LP64__)
    std::atomic_uint64_t m_initThreadsSize_{},
    m_idleThreadSize_{},
    m_threadsSizeThreshold_{MAX_THREADS_SIZE},
    m_tasksSizeThreshold_{MAX_TASKS_SIZE};
#else
    std::atomic_uint32_t m_initThreadsSize_{},
    m_idleThreadSize_{},
    m_threadsSizeThreshold_{},
    m_tasksSizeThreshold_{};
#endif

public:
    void start(const uint64_t &threadSize = std::thread::hardware_concurrency());

    void stop();

    XAbstractTask2_Ptr joinTask(const XAbstractTask2_Ptr &);

    void setMode(const Mode &mode){
        m_mode_ = mode;
    }

    inline void setThreadsSizeThreshold(const uint64_t &n){
        m_threadsSizeThreshold_ = n;
    }

    inline void setTasksSizeThreshold(const uint64_t &n){
        m_tasksSizeThreshold_ = n;
    }

    auto idleThreadsSize()const {
        return m_idleThreadSize_.load(std::memory_order_relaxed);
    }

    explicit XThreadPool2(const Mode &mode,Private);

    ~XThreadPool2();

    using XThreadPool2_Ptr = std::shared_ptr<XThreadPool2>;
    static XThreadPool2_Ptr create(const Mode &mode = Mode::FIXED);

private:
    void run(const idtype_t &threadId);
    X_DISABLE_COPY_MOVE(XThreadPool2)
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
