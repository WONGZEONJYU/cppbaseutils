#include "xabstractrunnable.hpp"
#include "xthreadpool.hpp"
#include <iostream>
#ifdef UNUSE_STD_THREAD_LOCAL
#include "xthreadlocal.hpp"
#endif

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XAbstractRunnablePrivate final : public XAbstractRunnableData {
    enum class Private{};
public:
    X_DECLARE_PUBLIC(XAbstractRunnable)
#ifdef UNUSE_STD_THREAD_LOCAL
    [[maybe_unused]] mutable XThreadLocalConstVoid m_isSelf{};
#else
    [[maybe_unused]] static inline thread_local const void * sm_isSelf{};
#endif
    mutable XAbstractRunnable::FuncVer m_is_OverrideConst{};
    mutable XAtomicBool m_recall{};
    mutable std::weak_ptr<XAbstractRunnable> m_next{};
    mutable std::function<bool()> m_is_running{};
    mutable XAtomicPointer<const void> m_owner{};

    explicit XAbstractRunnablePrivate(Private){}
    ~XAbstractRunnablePrivate() override = default;

    static auto create(){
        return std::make_shared<XAbstractRunnablePrivate>(Private{});
    }
};

XAbstractRunnable::XAbstractRunnable(const FuncVer &is_OverrideConst):
m_d_ptr_(XAbstractRunnablePrivate::create()) {
    X_D(XAbstractRunnable)
    d->m_x_ptr_ = this;
    d->m_is_OverrideConst = is_OverrideConst;
}

[[maybe_unused]] bool XAbstractRunnable::is_Running() const {
    X_D(const XAbstractRunnable)
    return d->m_is_running && d->m_is_running();
}

void XAbstractRunnable::call() const {
    X_D(const XAbstractRunnable)
    const XResultStorage ret(d->m_result_);
#ifdef UNUSE_STD_THREAD_LOCAL
    const XThreadLocalStorageConstVoid set_this(d->m_isSelf,this);
#else
    d->sm_isSelf = this;
#endif
    try {
        if (FuncVer::CONST == d->m_is_OverrideConst){
            ret.set(run());
        }else{
            auto &obj{const_cast<XAbstractRunnable &>(*this)};
            ret.set(obj.run());
        }
    } catch (const std::exception &e) {
        ret.set({});
        std::cerr << FUNC_SIGNATURE << " exception msg : " << e.what() << "\n";
    }
    d->m_is_running = {};
    d->m_owner.storeRelease({});
}

void XAbstractRunnable::resetRecall_() const {
    X_D(const XAbstractRunnable)
    d->m_result_.reset();
}

void XAbstractRunnable::set_exit_function_(std::function<bool()> &&f) const {
    X_D(const XAbstractRunnable)
    d->m_is_running = std::move(f);
}

XAtomicPointer<const void>& XAbstractRunnable::Owner_() const {
    X_D(const XAbstractRunnable)
    return d->m_owner;
}

[[maybe_unused]] void XAbstractRunnable::set_nextHandler(const std::weak_ptr<XAbstractRunnable>& next_) {
    X_D(const XAbstractRunnable)
    d->m_next = next_;
}

[[maybe_unused]] void XAbstractRunnable::requestHandler(const std::any& arg) {
    X_D(const XAbstractRunnable)
    if (const auto p{d->m_next.lock()}){
        p->responseHandler(arg);
    }
}

XAbstractRunnable_Ptr XAbstractRunnable::joinThreadPool(const XThreadPool_Ptr& pool) {
    const auto ret{shared_from_this()};
    if (pool){
        pool->runnableJoin(ret);
    }
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
