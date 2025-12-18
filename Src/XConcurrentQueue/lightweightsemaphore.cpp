#include <XConcurrentQueue/lwsemaphore_mach_p.hpp>
#include <XConcurrentQueue/lwsemaphore_unix_p.hpp>
#include <XConcurrentQueue/lwsemaphore_win_p.hpp>
#include <XAtomic/xatomic.hpp>

#define MOODYCAMEL_NAMESPACE_BEGIN namespace moodycamel {
#define MOODYCAMEL_NAMESPACE_END }

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)
MOODYCAMEL_NAMESPACE_BEGIN

class XLightweightSemaphorePrivate : public details::Semaphore {

public:
    using Base = Semaphore;

    X_DECLARE_PUBLIC(XLightweightSemaphore)
    XAtomicInteger<XLightweightSemaphore::ssize_t> m_count{};
    int m_maxSpins{};

    bool waitWithPartialSpinning(std::int64_t timeout_usecs = -1) noexcept;

    ssize_t waitManyWithPartialSpinning(ssize_t max, std::int64_t timeout_usecs = -1) noexcept;

    constexpr explicit XLightweightSemaphorePrivate(int const initialCount = {})
    :Base{initialCount} {}

    ~XLightweightSemaphorePrivate() override = default;
};

bool XLightweightSemaphorePrivate::waitWithPartialSpinning(std::int64_t const timeout_usecs ) noexcept {

    auto spin{ m_maxSpins };
    ssize_t oldCount{};
    while (--spin >= 0) {
        oldCount = m_count.loadRelaxed();
        if (oldCount > 0 && m_count.m_x_value.compare_exchange_strong(oldCount, oldCount - 1, std::memory_order_acquire, std::memory_order_relaxed))
        { return true;}
        std::atomic_signal_fence(std::memory_order_acquire);	 // Prevent the compiler from collapsing the loop.
    }

    oldCount = m_count.fetchAndSubAcquire(1);

    if (oldCount > 0) { return true; }

    if (timeout_usecs < 0)
    { if (wait()) { return true;} }

    if (timeout_usecs > 0 && timed_wait(static_cast<std::uint64_t>(timeout_usecs))) { return true; }
    // At this point, we've timed out waiting for the semaphore, but the
    // count is still decremented indicating we may still be waiting on
    // it. So we have to re-adjust the count, but only if the semaphore
    // wasn't signaled enough times for us too since then. If it was, we
    // need to release the semaphore too.
    while (true) {
        oldCount = m_count.loadAcquire();
        if (oldCount >= 0 && try_wait()) { return true; }
        if (oldCount < 0 && m_count.m_x_value.compare_exchange_strong(oldCount, oldCount + 1, std::memory_order_relaxed, std::memory_order_relaxed))
        { return {};}
    }
}

ssize_t XLightweightSemaphorePrivate::waitManyWithPartialSpinning(ssize_t const max, std::int64_t const timeout_usecs) noexcept {
    assert(max > 0);
    ssize_t oldCount{};
    auto spin { m_maxSpins };
    while (--spin >= 0) {
        oldCount = m_count.loadRelaxed();
        if (oldCount > 0) {
            if (auto const newCount{ oldCount > max ? oldCount - max : 0};
                m_count.m_x_value.compare_exchange_strong(oldCount, newCount, std::memory_order_acquire, std::memory_order_relaxed))
            {
                return oldCount - newCount;
            }
        }
        std::atomic_signal_fence(std::memory_order_acquire);
    }

    oldCount = m_count.fetchAndSubAcquire(1);

    if (oldCount <= 0) {
        if (!timeout_usecs
            || (timeout_usecs < 0 && !wait())
            || (timeout_usecs > 0 && !timed_wait(static_cast<std::uint64_t>(timeout_usecs))))
        {
            while (true) {
                oldCount = m_count.loadAcquire();
                if (oldCount >= 0 && try_wait()) { break; }
                if (oldCount < 0 && m_count.m_x_value.compare_exchange_strong(oldCount, oldCount + 1, std::memory_order_relaxed, std::memory_order_relaxed))
                { return 0;}
            }
        }
    }

    if (max > 1) { return 1 + x_func()->tryWaitMany(max - 1); }

    return 1;
}

XLightweightSemaphore::XLightweightSemaphore(ssize_t const initialCount,int const maxSpins)
    : m_d_ptr { std::make_unique<XLightweightSemaphorePrivate>(initialCount) }
{
    m_d_ptr->m_x_ptr = this;
    d_func()->m_maxSpins = maxSpins;
    assert(initialCount >= 0);
    assert(maxSpins >= 0);
}

bool XLightweightSemaphore::tryWait() noexcept {
    X_D(XLightweightSemaphore);
    auto oldCount{ d->m_count.loadRelaxed() };
    while (oldCount > 0) {
        if (d->m_count.m_x_value.compare_exchange_weak(oldCount, oldCount - 1
            , std::memory_order_acquire, std::memory_order_relaxed))
        { return true; }
    }
    return {};
}

bool XLightweightSemaphore::wait() noexcept
{ return tryWait() || d_func()->waitWithPartialSpinning(); }

bool XLightweightSemaphore::wait(std::int64_t const timeout_usecs) noexcept
{ return tryWait() || d_func()->waitWithPartialSpinning(timeout_usecs); }

// Acquires between 0 and (greedily) max, inclusive
ssize_t XLightweightSemaphore::tryWaitMany(ssize_t const max) noexcept {
    assert(max >= 0);
    X_D(XLightweightSemaphore);

    auto oldCount{ d->m_count.loadRelaxed() };
    while (oldCount > 0) {
        if (auto const newCount{oldCount > max ? oldCount - max : 0};
            d->m_count.m_x_value.compare_exchange_weak(oldCount, newCount, std::memory_order_acquire, std::memory_order_relaxed))
        {
            return oldCount - newCount;
        }
    }
    return {};
}

// Acquires at least one, and (greedily) at most max
ssize_t XLightweightSemaphore::waitMany(ssize_t const max, std::int64_t const timeout_usecs) noexcept {
    assert(max >= 0);
    auto result{ tryWaitMany(max) };
    if (!result && max > 0) { result = d_func()->waitManyWithPartialSpinning(max, timeout_usecs); }
    return result;
}

ssize_t XLightweightSemaphore::waitMany(ssize_t const max) noexcept {
    auto const result{ waitMany(max, -1) };
    assert(result > 0);
    return result;
}

void XLightweightSemaphore::signal(ssize_t const count) noexcept {
    assert(count >= 0);
    X_D(XLightweightSemaphore);
    auto const oldCount{ d->m_count.fetchAndAddRelaxed(count) };
    if (auto const toRelease { -oldCount < count ? -oldCount : count }
        ; toRelease > 0)
    { d->signal(static_cast<int>(toRelease)); }
}

std::size_t XLightweightSemaphore::availableApprox() const noexcept {
    auto const count{ d_func()->m_count.loadRelaxed() };
    return count > 0 ? static_cast<std::size_t>(count) : 0;
}

MOODYCAMEL_NAMESPACE_END
XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
