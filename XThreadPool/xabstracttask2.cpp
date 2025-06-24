#include "xabstracttask2.hpp"
#include "xthreadpool2.hpp"
#include <future>
#include <semaphore>

#ifdef UNUSE_STD_THREAD_LOCAL
#include "xthreadlocal.hpp"
#endif

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XAbstractTask2::XAbstractTask2Private final : public XAbstractTask2Data {
    enum class Private{};
public:
    X_DECLARE_PUBLIC(XAbstractTask2)

#if _LIBCPP_STD_VER >= 20
    mutable std::binary_semaphore m_allow_bin{0};
#else
    mutable Xbinary_Semaphore m_allow_bin{0};
#endif

#ifdef UNUSE_STD_THREAD_LOCAL
    mutable XThreadLocal<void *> m_isSelf{};
#else
    static inline thread_local const void * sm_isSelf{};
#endif
    mutable FuncVer m_is_OverrideConst{};
    mutable XAtomicBool m_occupy{};
    mutable std::promise<std::any> m_result{};
    mutable std::weak_ptr<XAbstractTask2> m_next{};
    mutable std::function<bool()> m_is_running{};
    mutable XAtomicPointer<const void> m_owner{};

    explicit XAbstractTask2Private(Private){}
    ~XAbstractTask2Private() override = default;

    static auto create(){
        return std::make_shared<XAbstractTask2Private>(Private{});
    }

    void setResult(std::any &&v) const {
        m_result = {};
        m_result.set_value(std::move(v));
        m_allow_bin.release();
    }
};

std::any XAbstractTask2::XAbstractTask2Data::getResult() const {
    const auto d{m_x_ptr_->d_func()};
    return d->m_result.get_future().get();
}

#if _LIBCPP_STD_VER >= 20
std::binary_semaphore&
#else
Xbinary_Semaphore& ()
#endif
XAbstractTask2::XAbstractTask2Data::operator()() const {
    const auto d{m_x_ptr_->d_func()};
    return d->m_allow_bin;
}

std::any XAbstractTask2::XAbstractTask2Data::result_(const Model & m) const {
    const auto *const d{m_x_ptr_->d_func()};
    constexpr std::string_view selfname{__PRETTY_FUNCTION__};
#if 0
    const XRAII raii{[&selfname]{
        std::cout << selfname << " begin\n" << std::flush;
    },[&selfname]{
        std::cout << selfname << " end\n" << std::flush;
    }};
#endif
    if (NonblockModel == m){
        return d->m_allow_bin.try_acquire() ? d->m_occupy.storeRelease(true),
        d->m_result.get_future().get() : std::any{};
    }

#ifdef UNUSE_STD_THREAD_LOCAL
    if (this == d->m_isSelf.get().value_or(nullptr)){
#else
    if (this == d->sm_isSelf){
#endif
        std::cerr << selfname << " tips: Working Thread Call invalid\n" << std::flush;
        return {};
    }

    if (!d->m_is_running){
        std::cerr << selfname << " tips: tasks not added\n" << std::flush;
        return {};
    }

    if (d->m_occupy.loadAcquire()){
        std::cerr << selfname << " tips: Repeated calls\n" << std::flush;
        return {};
    }
    d->m_occupy.storeRelease(true);

    d->m_allow_bin.acquire();

    return d->m_result.get_future().get();
}

std::any XAbstractTask2::XAbstractTask2Data::result_for_(const std::chrono::nanoseconds &rel_time) const {
    const auto *const d{m_x_ptr_->d_func()};
    constexpr std::string_view selfname{__PRETTY_FUNCTION__};
#if 0
    const XRAII raii{[&selfname]{
        std::cout << selfname << " begin\n" << std::flush;
    },[&selfname]{
        std::cout << selfname << " end\n" << std::flush;
    }};
#endif
    if (!d->m_is_running){
        std::cerr << selfname << " tips: tasks not added\n" << std::flush;
        return {};
    }

    if (!d->m_allow_bin.try_acquire_for(rel_time)){
        std::cerr << selfname << " tips: timeout\n" << std::flush;
        return {};
    }

    return d->m_result.get_future().get();
}

XAbstractTask2::XAbstractTask2(const FuncVer &is_OverrideConst):
m_d_ptr_(XAbstractTask2Private::create()) {
    X_D(XAbstractTask2);
    d->m_x_ptr_ = this;
    d->m_is_OverrideConst = is_OverrideConst;
}

bool XAbstractTask2::is_running_() const {
    X_D(const XAbstractTask2);
    return d->m_is_running && d->m_is_running();
}

void XAbstractTask2::operator()() {
    X_D(XAbstractTask2);
    std::any ret_val{};
    try {
#ifdef UNUSE_STD_THREAD_LOCAL
        XThreadLocalRaii<void*> set(d->m_isSelf,this);
#else
        d->sm_isSelf = this;
#endif
        ret_val = std::move(run());
    } catch (const std::exception &e) {
        ret_val.reset();
        std::cerr << __PRETTY_FUNCTION__ << " exception msg : " << e.what() << "\n";
    }
    d->setResult(std::move(ret_val));
    d->m_owner.storeRelease({});
}

void XAbstractTask2::operator()() const {
    X_D(const XAbstractTask2);
    std::any ret_val{};
    try {
#ifdef UNUSE_STD_THREAD_LOCAL
        XThreadLocalRaii<void*> set(d->m_isSelf,this);
#else
        d->sm_isSelf = this;
#endif
        ret_val = std::move(run());
    } catch (const std::exception &e) {
        ret_val.reset();
        std::cerr << __PRETTY_FUNCTION__ << " exception msg : " << e.what() << "\n";
    }
    d->setResult(std::move(ret_val));
    d->m_owner.storeRelease({});
}

bool XAbstractTask2::is_OverrideConst() const {
    X_D(const XAbstractTask2);
    return CONST_RUN == d->m_is_OverrideConst;
}

void XAbstractTask2::resetOccupy_() const {
    X_D(const XAbstractTask2);
    d->m_occupy.storeRelease({});
}

void XAbstractTask2::set_exit_function_(std::function<bool()> &&f) const {
    X_D(const XAbstractTask2);
    d->m_is_running = std::move(f);
}

XAtomicPointer<const void>& XAbstractTask2::Owner() const {
    X_D(const XAbstractTask2);
    return d->m_owner;
}

[[maybe_unused]] void XAbstractTask2::set_nextHandler(const std::weak_ptr<XAbstractTask2>& next_) {
    X_D(const XAbstractTask2);
    d->m_next = next_;
}

[[maybe_unused]] void XAbstractTask2::requestHandler(const std::any& arg) {
    X_D(const XAbstractTask2);
    if (const auto p{d->m_next.lock()}){
        p->responseHandler(arg);
    }
}

XAbstractTask2_Ptr XAbstractTask2::joinThreadPool(const XThreadPool2_Ptr& pool) {
    const auto ret{shared_from_this()};
    if (pool){
        pool->taskJoin(ret);
    }
    return ret;
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
