#include "xabstractrunnable_p.hpp"
#include "xthreadpool.hpp"
#include <iostream>

#ifdef UNUSE_STD_THREAD_LOCAL
#include "xthreadlocal.hpp"
#endif

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

#ifndef UNUSE_STD_THREAD_LOCAL
    static thread_local const void * sm_isSelf{};
#endif

XAbstractRunnable::XAbstractRunnable(FuncVer const is_OverrideConst)
: m_d_ptr_ {std::make_shared<XAbstractRunnablePrivate>() }
{
    X_D(XAbstractRunnable);
    d->m_x_ptr = this;
    d->m_is_OverrideConst = is_OverrideConst;
}

[[maybe_unused]] bool XAbstractRunnable::is_Running() const noexcept
{ X_D(const XAbstractRunnable); return d->m_is_running && d->m_is_running(); }

void XAbstractRunnable::call() const {
    X_D(const XAbstractRunnable);
    XResultStorage const ret { d->m_result_ };
#ifdef UNUSE_STD_THREAD_LOCAL
    XThreadLocalStorageConstVoid const set_this(d->m_isSelf,this);
#else
    sm_isSelf = this;
#endif
    try {
        if (FuncVer::CONST == d->m_is_OverrideConst) {
            ret.set(run());
        }else{
            auto & obj{ const_cast<XAbstractRunnable &>(*this) };
            ret.set(obj.run());
        }
    } catch (const std::exception &e) {
        ret.set({});
        std::cerr << FUNC_SIGNATURE << " exception msg : " << e.what() << "\n";
    }
    d->m_is_running = {};
    d->m_owner.storeRelease({});
}

void XAbstractRunnable::resetRecall_() const noexcept
{ d_func()->m_result_.reset(); }

void XAbstractRunnable::allow_get_() const noexcept
{ d_func()->m_result_.allow_get(); }

void XAbstractRunnable::set_exit_function_(std::function<bool()> &&f) const noexcept
{ d_func()->m_is_running = std::move(f); }

XAtomicPointer<const void> & XAbstractRunnable::Owner_() const noexcept
{ return d_func()->m_owner; }

XAbstractRunnablePtr XAbstractRunnable::joinThreadPool(XThreadPoolPtr const & pool) noexcept {
    auto ret{ shared_from_this() };
    if (pool){ pool->runnableJoin(ret); }
    return ret;
}

std::any XAbstractRunnable::run() {
    std::cerr <<
        FUNC_SIGNATURE << " tips: You did not rewrite this function, "
        "please pay attention to whether your constructor parameters are filled in incorrectly\n";
    return {};
}

std::any XAbstractRunnable::run() const {
    std::cerr <<
      FUNC_SIGNATURE << " tips: You did not rewrite this function, "
        "please pay attention to whether your constructor parameters are filled in incorrectly\n";
    return {};
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
