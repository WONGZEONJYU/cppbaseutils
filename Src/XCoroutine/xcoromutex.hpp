#ifndef XUTILS2_X_CORO_MUTEX_HPP_
#define XUTILS2_X_CORO_MUTEX_HPP_ 1

#include <mutex>
#include <deque>
#include <atomic>
#include <coroutine>
#include <XGlobal/xversion.hpp>
#include <XGlobal/xclasshelpermacros.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XCoroMutex final {

    std::atomic_bool mutable m_locked{};
    std::deque<std::coroutine_handle<>> mutable m_handles{};
    std::mutex mutable m_mutex{};

    class AWaiter {
        XCoroMutex const * const m_mtx_{};
    public:
        X_IMPLICIT AWaiter(XCoroMutex const *) noexcept;
        [[nodiscard]] bool await_ready() const noexcept;
        void await_suspend(std::coroutine_handle<>) const noexcept;
        static constexpr void await_resume() noexcept {}
    };

public:
    explicit XCoroMutex();
    ~XCoroMutex();
    [[nodiscard]] bool try_lock() const noexcept;
    [[nodiscard]] AWaiter lock() const;
    void unlock() const noexcept;
    X_DISABLE_COPY(XCoroMutex)
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
