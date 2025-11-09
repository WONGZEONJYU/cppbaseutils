#include <taskthread.hpp>

TaskThread::~TaskThread() {
    m_is_exit_.storeRelease(true);
    m_cv_.notify_all();
}

void TaskThread::run() {

    while (!m_is_exit_.loadAcquire()) {
        std::unique_lock qlk{m_queueMtx_},cvlk{ m_cvMtx_ };

        while (!m_is_exit_.loadAcquire() && m_taskQueue_.empty()) {
            qlk.unlock();
            m_cv_.wait(cvlk);
            qlk.lock();
        }

        if (m_taskQueue_.empty()) { continue; }
        auto const task{ m_taskQueue_.front() };
        m_taskQueue_.pop();
        qlk.unlock();
        cvlk.unlock();
        if (task) { task->run(); }
    }
}

void TaskThread::putTask(std::shared_ptr<Runnable> const & task) {

    std::unique_lock qlk{ m_queueMtx_ } ,cvlk{ m_cvMtx_ };
    m_taskQueue_.push(task);
    m_cv_.notify_all();
}
