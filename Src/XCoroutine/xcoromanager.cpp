#include <xcoromanager.hpp>

XUtils::XCoroManager & XUtils::XCoroManager::instance() {
    static XCoroManager mgr{};
    return mgr;
}

std::size_t XUtils::XCoroManager::onlineSize() const noexcept {
    std::shared_lock lk{ m_setMtx_ };
    return m_handles_.size();
}

void XUtils::XCoroManager::isAllDone() const {

    // auto const cb{ [this]()->std::function<void()> {
    //     std::shared_lock setLk {m_setMtx_ },fnLk {m_fnMtx_ };
    //     if (m_handles_.empty() && m_callback_)
    //     { return m_callback_; }
    //     return {};
    // }() };
    //
    // if (cb) {
    //     try { cb(); }
    //     catch (std::exception const &) {}
    //     setCallback(std::function<void()>{});
    // }
}

XUtils::XCoroManager::XCoroManager() = default;

bool XUtils::XCoroManager::addHandle(std::coroutine_handle<> const & h) const noexcept {
    std::unique_lock lk{ m_setMtx_ };
    auto const ok { m_handles_.insert(h).second };
    if (ok) { m_online_.ref(); }
    return ok;
}

void XUtils::XCoroManager::removeHandle(std::coroutine_handle<> const & h) const noexcept {
    {
        std::unique_lock lk{ m_setMtx_ };
        if (m_handles_.erase(h) > 0) { m_online_.deref(); }
    }
    isAllDone();
}
