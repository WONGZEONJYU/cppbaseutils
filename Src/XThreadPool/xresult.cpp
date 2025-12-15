#include "xresult_p.hpp"
#include <iostream>

#ifdef UNUSE_STD_THREAD_LOCAL
#include "xthreadlocal.hpp"
#endif

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

#ifndef UNUSE_STD_THREAD_LOCAL
    static thread_local const void * sm_isSelf{};
#endif

std::any XResultPrivate::get_value() const
{ return m_result_.get_future().get(); }

XResult::XResult():
m_d_ptr_{std::make_unique<XResultPrivate>()}
{ m_d_ptr_->m_x_ptr = this; }

void XResult::set(std::any && v) const noexcept {
    X_D(const XResult);
    d->m_result_ = {};
    d->m_result_.set_value(std::move(v));
    d->m_bin_sem_.release();
}

void XResult::reset() const noexcept
{ d_func()->m_recall_.storeRelease({}); }

void XResult::allow_get() const noexcept
{ d_func()->m_allow_get_.storeRelease(true); }

std::any XResult::get_() const noexcept {

#ifndef UNUSE_STD_THREAD_LOCAL
    if (this == sm_isSelf) {
#else
    if (this == d->m_isSelf().value_or(nullptr)){
#endif
        std::cerr << FUNC_SIGNATURE << " tips: Working Thread Call invalid\n" << std::flush;
        return {};
    }

    X_D(const XResult);

    if (d->m_recall_.loadAcquire()){
        std::cerr << FUNC_SIGNATURE << " tips: Repeated calls\n" << std::flush;
        return {};
    }
    d->m_recall_.storeRelease(true);

    if (!d->m_allow_get_){
        std::cerr << FUNC_SIGNATURE << " tips: not allow get\n" << std::flush;
        return {};
    }
    d->m_allow_get_.storeRelease({});

    d->m_bin_sem_.acquire();
    return d->get_value();
}

std::any XResult::try_get_() const noexcept {
    X_D(const XResult);
    if (!d->m_bin_sem_.try_acquire()) { return {}; }
    return d->get_value();
}

std::any XResult::get_for_(std::chrono::nanoseconds const &del_time) const noexcept {
    X_D(const XResult);
    if (d->m_bin_sem_.try_acquire_for(del_time)){ return {}; }
    return d->get_value();
}

XResultStorage::XResultStorage(XResult & obj):
m_XResult_(std::addressof(obj)) {
#ifdef UNUSE_STD_THREAD_LOCAL
    auto const d { m_XResult_->d_func()};
    XThreadLocalStorageConstVoid const_ set { d->m_isSelf,m_XResult_,true };
#else
    sm_isSelf = m_XResult_;
#endif
}

[[maybe_unused]] XResultStorage::XResultStorage(XResult const & obj):
XResultStorage(const_cast<XResult &>(obj)) {}

void XResultStorage::set(std::any && v) const noexcept
{ m_XResult_->set(std::move(v)); }

void XResultStorage::release() const noexcept {
    [[maybe_unused]] auto const d{ m_XResult_->d_func() };
#ifdef UNUSE_STD_THREAD_LOCAL
    d->m_isSelf.remove_value();
#endif
}

XResultStorage::~XResultStorage()
{ release(); }

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
