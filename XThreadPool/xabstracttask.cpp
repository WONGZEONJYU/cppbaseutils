#include "xabstracttask.hpp"
#include "xthreadpool.hpp"
#include <future>
#include <semaphore>

#include "xthreadlocal.hpp"

#ifdef UNUSE_STD_THREAD_LOCAL
#include "xthreadlocal.hpp"
#endif

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XAbstractTask::XAbstractTaskPrivate final : public XAbstractTaskData {
    enum class Private{};
public:
    X_DECLARE_PUBLIC(XAbstractTask)
#ifdef UNUSE_STD_THREAD_LOCAL
    mutable XThreadLocalConstVoid m_isSelf{};
#else
    static inline thread_local const void * sm_isSelf{};
#endif
    mutable FuncVer m_is_OverrideConst{};
    mutable XAtomicBool m_recall{};
    mutable std::promise<std::any> m_result{};
    mutable std::weak_ptr<XAbstractTask> m_next{};
    mutable std::function<bool()> m_is_running{};
    mutable XAtomicPointer<const void> m_owner{};

    explicit XAbstractTaskPrivate(Private){}
    ~XAbstractTaskPrivate() override = default;

    static auto create(){
        return std::make_shared<XAbstractTaskPrivate>(Private{});
    }

    std::any getResult() const override {
        return std::move(m_result.get_future().get());
    }

    std::any result_(const Model & m) const override {

        if (NonblockModel == m){
            return m_allow_bin.try_acquire() ? m_recall.storeRelease(true),
            std::move(getResult()) : std::any{};
        }

#ifdef UNUSE_STD_THREAD_LOCAL
        if (this == m_isSelf.get().value_or(nullptr)){
#else
        if (this == sm_isSelf){
#endif
            std::cerr << __PRETTY_FUNCTION__ << " tips: Working Thread Call invalid\n" << std::flush;
            return {};
        }

        if (!m_is_running){
            std::cerr << __PRETTY_FUNCTION__ << " tips: tasks not added\n" << std::flush;
            return {};
        }

        if (m_recall.loadAcquire()){
            std::cerr << __PRETTY_FUNCTION__ << " tips: Repeated calls\n" << std::flush;
            return {};
        }
        m_recall.storeRelease(true);

        m_allow_bin.acquire();

        return std::move(getResult());
    }

    std::any result_for_(const std::chrono::nanoseconds &rel_time) const override {

        if (!m_is_running){
            std::cerr << __PRETTY_FUNCTION__ << " tips: tasks not added\n" << std::flush;
            return {};
        }

        if (!m_allow_bin.try_acquire_for(rel_time)){
            std::cerr << __PRETTY_FUNCTION__ << " tips: timeout\n" << std::flush;
            return {};
        }

        return std::move(getResult());
    }

    void setResult(std::any &&v) const {
        m_result = {};
        m_result.set_value(std::move(v));
        m_allow_bin.release();
    }
};

XAbstractTask::XAbstractTask(const FuncVer &is_OverrideConst):
m_d_ptr_(XAbstractTaskPrivate::create()) {
    X_D(XAbstractTask);
    d->m_x_ptr_ = this;
    d->m_is_OverrideConst = is_OverrideConst;
}

bool XAbstractTask::is_running() const {
    X_D(const XAbstractTask);
    return d->m_is_running && d->m_is_running();
}

void XAbstractTask::operator()() const {
    X_D(const XAbstractTask);
    //std::any ret_val{};
    const XSetResult set(d->m_result_);

    try {
#ifdef UNUSE_STD_THREAD_LOCAL
        XThreadLocalRaiiConstVoid set(d->m_isSelf,this);
#else
        d->sm_isSelf = this;
#endif
        if (CONST_RUN == d->m_is_OverrideConst){
            //ret_val = std::move(run());
            set.set(std::move(run()));
        }else{
            auto &obj{const_cast<XAbstractTask &>(*this)};
            //ret_val = std::move(obj.run());
            set.set(std::move(obj.run()));
        }
    } catch (const std::exception &e) {
        set.set({});
        std::cerr << __PRETTY_FUNCTION__ << " exception msg : " << e.what() << "\n";
    }
    d->m_owner.storeRelease({});
}

void XAbstractTask::resetRecall_() const {
    X_D(const XAbstractTask);
    d->m_result_.reset();
}

void XAbstractTask::set_exit_function_(std::function<bool()> &&f) const {
    X_D(const XAbstractTask);
    d->m_is_running = std::move(f);
}

XAtomicPointer<const void>& XAbstractTask::Owner_() const {
    X_D(const XAbstractTask);
    return d->m_owner;
}

[[maybe_unused]] void XAbstractTask::set_nextHandler(const std::weak_ptr<XAbstractTask>& next_) {
    X_D(const XAbstractTask);
    d->m_next = next_;
}

[[maybe_unused]] void XAbstractTask::requestHandler(const std::any& arg) {
    X_D(const XAbstractTask);
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
