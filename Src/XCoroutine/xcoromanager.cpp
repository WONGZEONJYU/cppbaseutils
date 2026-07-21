#include <xcoromanager.hpp>
#include <XAtomic/xatomic.hpp>
#include <unordered_set>
#include <shared_mutex>

#include "xlog.hpp"

XTD_NAMESPACE_BEGIN
    XTD_INLINE_NAMESPACE_BEGIN(v1)

class XCoroManagerPrivate final {

public:
    XCoroManager * m_x_ptr{};
    mutable std::unordered_set<std::coroutine_handle<>> m_handles_{};
    mutable detail::cb_t m_callback_{};
    mutable std::shared_mutex m_setMtx_{},m_fnMtx_{};
    mutable XAtomicInteger<std::size_t> m_online_{};

    X_DECLARE_PUBLIC(XCoroManager)
    explicit XCoroManagerPrivate(XCoroManager * const x) : m_x_ptr{x}
    {   }
    ~XCoroManagerPrivate() = default;
};

XCoroManager & XCoroManager::instance() {
    static XCoroManager mgr{};
    return mgr;
}

std::size_t XCoroManager::onlineSize() const noexcept {
    X_D(const XCoroManager);
#if 0
    std::shared_lock lk{ d->m_setMtx_ };
    return d->m_handles_.size();
#else
    return d->m_online_.loadAcquire();
#endif
}

XCoroManager::~XCoroManager() = default;

XCoroManager::XCoroManager()
    :m_d_ptr_{ std::make_unique<XCoroManagerPrivate>(this) }
{   }

bool XCoroManager::add(std::coroutine_handle<> const & h) const noexcept {
    X_D(const XCoroManager);
    std::unique_lock lk{ d->m_setMtx_ };
    auto const ok{ d->m_handles_.insert(h).second };
    if (ok) { d->m_online_.ref(); }
    return ok;
}

void XCoroManager::remove(std::coroutine_handle<> const & h) const noexcept {
    X_D(const XCoroManager);

    if (![d,&h]() noexcept{
        std::unique_lock lk{ d->m_setMtx_ };
        if (d->m_handles_.erase(h) > 0) { d->m_online_.deref(); }
        return d->m_handles_.empty();
    }()) { return; }

    auto const cb{ [d]() noexcept{
        std::unique_lock lk{ d->m_fnMtx_ };
        auto fn{ std::move(d->m_callback_) };
        return fn;
    }()};

    if (!cb) { return; }
    try { cb(); }
    catch (std::exception const & e) { XLOG_FATAL(e.what()); }
}

void XCoroManager::setAllExitCallback(detail::cb_t && cb) const noexcept{
    X_D(const XCoroManager);
    std::unique_lock lk { d->m_fnMtx_ };
    d->m_callback_.swap(cb);
}

XCoroManager * coroMgrPtr() noexcept
{ return std::addressof(XCoroManager::instance()); }

XCoroManager & coroMgrRef() noexcept
{ return XCoroManager::instance(); }

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
