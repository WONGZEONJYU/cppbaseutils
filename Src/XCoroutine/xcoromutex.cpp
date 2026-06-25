#include <xcoromutex.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

XCoroMutex::AWaiter::AWaiter(XCoroMutex const * const mtx) noexcept
    :m_mtx_{ mtx }
{   }

bool XCoroMutex::AWaiter::await_ready() const noexcept
{ return m_mtx_->try_lock(); }

void XCoroMutex::AWaiter::await_suspend(std::coroutine_handle<> const h) const noexcept {
    std::unique_lock lk { m_mtx_->m_mutex };
    m_mtx_->m_handles.push_back(h);
}

XCoroMutex::XCoroMutex() = default;

XCoroMutex::~XCoroMutex() = default;

bool XCoroMutex::try_lock() const noexcept {
    bool expected{};
    return m_locked.compare_exchange_strong(expected,true,std::memory_order_acquire);
}

XCoroMutex::AWaiter XCoroMutex::lock() const
{ return this; }

void XCoroMutex::unlock() const noexcept {
    auto const h { [this]{
        std::unique_lock lk { m_mutex };
        if (m_handles.empty()) { return std::coroutine_handle{}; }
        auto const handle { m_handles.front() };
        m_handles.pop_front();
        return handle;
    }() };

    h ? h() : m_locked.store({},std::memory_order_release);
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
