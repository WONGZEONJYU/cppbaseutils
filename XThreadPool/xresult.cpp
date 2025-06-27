#include <XAtomic/xatomic.hpp>
#include "xresult.hpp"
#include <iostream>

#ifdef UNUSE_STD_THREAD_LOCAL
#include "xthreadlocal.hpp"
#endif

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XResultPrivate final : public XResultData {
    X_DISABLE_COPY_MOVE(XResultPrivate)
public:
    X_DECLARE_PUBLIC(XResult)
    mutable std::promise<std::any> m_result_{};
    mutable XAtomicBool m_recall_{},m_allow_get_{};
#ifdef UNUSE_STD_THREAD_LOCAL
    mutable XThreadLocalConstVoid m_isSelf{};
#else
    static inline thread_local const void * sm_isSelf{};
#endif
    explicit XResultPrivate() = default;
    ~XResultPrivate() override = default;
    std::any get_value() const override {
        return std::move(m_result_.get_future().get());
    }
};

XResult::XResult():
m_d_ptr_(std::move(std::make_unique<XResultPrivate>())){
    m_d_ptr_->m_x_ptr_ = this;
}

void XResult::set(std::any && v) const {
    X_D(const XResult);
    d->m_result_ = {};
    d->m_result_.set_value(std::move(v));
    d->m_bin_sem_.release();
}

void XResult::reset() const {
    X_D(const XResult);
    d->m_recall_.storeRelease({});
}

void XResult::allow_get() const{
    X_D(const XResult);
    d->m_allow_get_.storeRelease(true);
}

std::any XResult::get_() const {
    X_D(const XResult);
#ifndef UNUSE_STD_THREAD_LOCAL
    if (this == d->sm_isSelf){
#else
    if (this == d->m_isSelf().value_or(nullptr)){
#endif
        std::cerr << __PRETTY_FUNCTION__ << " tips: Working Thread Call invalid\n" << std::flush;
        return {};
    }

    if (d->m_recall_.loadAcquire()){
        std::cerr << __PRETTY_FUNCTION__ << " tips: Repeated calls\n" << std::flush;
        return {};
    }
    d->m_recall_.storeRelease(true);

    if (!d->m_allow_get_){
        std::cerr << __PRETTY_FUNCTION__ << " tips: not allow get\n" << std::flush;
        return {};
    }
    d->m_allow_get_.storeRelease({});

    d->m_bin_sem_.acquire();
    return std::move(d->get_value());
}

std::any XResult::try_get_() const {
    X_D(const XResult);
    if (!d->m_bin_sem_.try_acquire()){
        return {};
    }
    return std::move(d->get_value());
}

std::any XResult::get_for_(std::chrono::nanoseconds const &del_time) const {
    X_D(const XResult);
    if (d->m_bin_sem_.try_acquire_for(del_time)){
        return {};
    }
    return std::move(d->get_value());
}

XResultStorage::XResultStorage(XResult & obj):
m_XResult_(std::addressof(obj)) {
    const auto d{m_XResult_->d_func()};
#ifdef UNUSE_STD_THREAD_LOCAL
    const XThreadLocalStorageConstVoid set(d->m_isSelf,m_XResult_,true);
#else
    d->sm_isSelf = m_XResult_;
#endif
}

XResultStorage::XResultStorage(const XResult &obj):
XResultStorage(const_cast<XResult &>(obj)){}

void XResultStorage::set(std::any&& v) const {
    m_XResult_->set(std::move(v));
}

void XResultStorage::release() const {
    const auto d{m_XResult_->d_func()};
    (void)d;
#ifdef UNUSE_STD_THREAD_LOCAL
    d->m_isSelf.remove_value();
#endif
}

XResultStorage::~XResultStorage(){
    release();
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
