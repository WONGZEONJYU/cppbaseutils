#ifndef XUTILS2_TASKTHREAD_HPP
#define XUTILS2_TASKTHREAD_HPP

#include <queue>
#include <mutex>
#include <condition_variable>
#include <runnable.hpp>
#include <mythread.hpp>
#include <XAtomic/xatomic.hpp>

class TaskThread final : public Runnable {

    std::mutex m_queueMtx_{},m_cvMtx_{};
    std::queue<std::shared_ptr<Runnable>> m_taskQueue_{};
    std::condition_variable_any m_cv_{};
    MyThread m_t_{this};
    XUtils::XAtomicBool m_is_exit_{};

public:
    TaskThread() = default;
    ~TaskThread() override;
    void run() override;
    void putTask(std::shared_ptr<Runnable> const & );
};

#endif
