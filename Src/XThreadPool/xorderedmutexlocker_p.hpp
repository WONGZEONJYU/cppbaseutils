#ifndef X_ORDERED_MUTEX_LOCKER_HPP
#define X_ORDERED_MUTEX_LOCKER_HPP 1

#include <XHelper/xhelper.hpp>
#include <mutex>

class XOrderedMutexLocker {
    std::mutex *m_mtx1_{},*m_mtx2_{};
    bool m_locked_{};
public:
    XOrderedMutexLocker(std::mutex * const m1, std::mutex * const m2)
        : m_mtx1_(m1 == m2 ? m1 : std::less<>()(m1, m2) ? m1 : m2),
          m_mtx2_(m1 == m2 ?  nullptr : std::less<>()(m1, m2) ? m2 : m1) {
        relock();
    }

    X_DISABLE_COPY(XOrderedMutexLocker)

    void swap(XOrderedMutexLocker &other)  noexcept {
        std::swap(m_mtx1_, other.m_mtx1_);
        std::swap(m_mtx2_, other.m_mtx2_);
        std::swap(m_locked_, other.m_locked_);
    }

    XOrderedMutexLocker& operator=(XOrderedMutexLocker&& other) noexcept {
        XOrderedMutexLocker moved (std::move(other));
        swap(moved);
        return *this;
    }

    XOrderedMutexLocker(XOrderedMutexLocker &&other) noexcept
        : m_mtx1_(std::exchange(other.m_mtx1_, {}))
        , m_mtx2_(std::exchange(other.m_mtx2_, {}))
        , m_locked_(std::exchange(other.m_locked_, {}))
    {}

    ~XOrderedMutexLocker() {
        unlock();
    }

    void relock() {
        if (!m_locked_) {
            if (m_mtx1_) {m_mtx1_->lock();}
            if (m_mtx2_) {m_mtx2_->lock();}
            m_locked_ = true;
        }
    }

    [[maybe_unused]] void dismiss() {
        X_ASSERT(m_locked_);
        m_locked_ = false;
    }

    void unlock() {
        if (m_locked_) {
            if (m_mtx2_) m_mtx2_->unlock();
            if (m_mtx1_) m_mtx1_->unlock();
            m_locked_ = false;
        }
    }

    [[maybe_unused]] inline static bool relock(std::mutex * const mtx1, std::mutex * const mtx2) {
        // mtx1 is already locked, mtx2 not... do we need to unlock and relock?
        if (mtx1 == mtx2){
            return {};
        }

        if (std::less<std::mutex *>{}(mtx1, mtx2)) {
            mtx2->lock();
            return true;
        }

        if (!mtx2->try_lock()) {
            mtx1->unlock();
            mtx2->lock();
            mtx1->lock();
        }
        return true;
    }
};

#endif
