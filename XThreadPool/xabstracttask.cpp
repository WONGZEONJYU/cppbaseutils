#include "xabstracttask.hpp"
#include "xthreadpool.hpp"
#ifdef UNUSE_STD_THREAD_LOCAL
#include "xthreadlocal.hpp"
#endif

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XAbstractTaskPrivate final : public XAbstractTaskData {
    enum class Private{};
public:
    X_DECLARE_PUBLIC(XAbstractTask)
#ifdef UNUSE_STD_THREAD_LOCAL
    mutable XThreadLocalConstVoid m_isSelf{};
#else
    static inline thread_local const void * sm_isSelf{};
#endif
    mutable XAbstractTask::FuncVer m_is_OverrideConst{};
    mutable XAtomicBool m_recall{};
    mutable std::weak_ptr<XAbstractTask> m_next{};
    mutable std::function<bool()> m_is_running{};
    mutable XAtomicPointer<const void> m_owner{};

    explicit XAbstractTaskPrivate(Private){}
    ~XAbstractTaskPrivate() override = default;

    static auto create(){
        return std::make_shared<XAbstractTaskPrivate>(Private{});
    }
};

XAbstractTask::XAbstractTask(const FuncVer &is_OverrideConst):
m_d_ptr_(XAbstractTaskPrivate::create()) {
    X_D(XAbstractTask)
    d->m_x_ptr_ = this;
    d->m_is_OverrideConst = is_OverrideConst;
}

bool XAbstractTask::is_running() const {
    X_D(const XAbstractTask)
    return d->m_is_running && d->m_is_running();
}

void XAbstractTask::call() const {
    X_D(const XAbstractTask)
    const XResultStorage ret(d->m_result_);
#ifdef UNUSE_STD_THREAD_LOCAL
    const XThreadLocalStorageConstVoid set_this(d->m_isSelf,this);
#else
    d->sm_isSelf = this;
#endif
    try {
        if (CONST_RUN == d->m_is_OverrideConst){
            ret.set(std::move(run()));
        }else{
            auto &obj{const_cast<XAbstractTask &>(*this)};
            ret.set(std::move(obj.run()));
        }
    } catch (const std::exception &e) {
        ret.set({});
        std::cerr << __PRETTY_FUNCTION__ << " exception msg : " << e.what() << "\n";
    }
    d->m_is_running = {};
    d->m_owner.storeRelease({});
}

void XAbstractTask::resetRecall_() const {
    X_D(const XAbstractTask)
    d->m_result_.reset();
}

void XAbstractTask::set_exit_function_(std::function<bool()> &&f) const {
    X_D(const XAbstractTask)
    d->m_is_running = std::move(f);
}

XAtomicPointer<const void>& XAbstractTask::Owner_() const {
    X_D(const XAbstractTask)
    return d->m_owner;
}

[[maybe_unused]] void XAbstractTask::set_nextHandler(const std::weak_ptr<XAbstractTask>& next_) {
    X_D(const XAbstractTask)
    d->m_next = next_;
}

[[maybe_unused]] void XAbstractTask::requestHandler(const std::any& arg) {
    X_D(const XAbstractTask)
    if (const auto p{d->m_next.lock()}){
        p->responseHandler(arg);
    }
}

XAbstractTask_Ptr XAbstractTask::joinThreadPool(const XThreadPool_Ptr& pool) {
    const auto ret{shared_from_this()};
    if (pool){
        pool->taskJoin(ret);
    }
    return ret;
}

std::any XAbstractTask::run() {
    std::cerr <<
        __PRETTY_FUNCTION__ << " tips: You did not rewrite this function, "
        "please pay attention to whether your constructor parameters are filled in incorrectly\n";
    return {};
}

std::any XAbstractTask::run() const {
    std::cerr <<
        __PRETTY_FUNCTION__ << " tips: You did not rewrite this function, "
        "please pay attention to whether your constructor parameters are filled in incorrectly\n";
    return {};
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
