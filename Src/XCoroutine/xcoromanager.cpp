#include <xcoromanager.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

XUtils::XCoroManager & XUtils::XCoroManager::instance() {
    static XCoroManager mgr{};
    return mgr;
}

std::size_t XUtils::XCoroManager::onlineSize() const noexcept {
    std::shared_lock lk{ m_setMtx_ };
    return m_handles_.size();
}

XUtils::XCoroManager::XCoroManager() = default;

bool XUtils::XCoroManager::addHandle(std::coroutine_handle<> const & h) const noexcept {
    std::unique_lock lk{ m_setMtx_ };
    auto const ok{ m_handles_.insert(h).second };
    if (ok) { m_online_.ref(); }
    return ok;
}

void XUtils::XCoroManager::removeHandle(std::coroutine_handle<> const & h) const noexcept {

    auto const needCallBack{ [this,&h]()noexcept{
        std::unique_lock lk{ m_setMtx_ };
        if (m_handles_.erase(h) > 0) { m_online_.deref(); }
        return m_handles_.empty();
    }()};

    if (!needCallBack) { return; }

    auto const cb{ [this]() noexcept{
        std::unique_lock lk{ m_fnMtx_ };
        auto fn{ std::move(m_callback_) };
        return fn;
    }()};

    if (!cb) { return; }
    try { cb(); } catch (std::exception const &) {}
}

XCoroManager & coroMgrRef() noexcept
{ return XCoroManager::instance(); }

XCoroManager * coroMgrPtr() noexcept
{ return std::addressof(coroMgrRef()); }

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
