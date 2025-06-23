#include "xabstracttask2.hpp"
#include "xthreadpool2.hpp"
#include <future>
#include <XAtomic/xatomic.hpp>
#include <semaphore>

#ifdef UNUSE_STD_THREAD_LOCAL
#include "xthreadlocal.hpp"
#endif

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XAbstractTask2::XAbstractTask2Private final {
    enum class Private{};
public:
#ifdef UNUSE_STD_THREAD_LOCAL
    mutable XThreadLocal<void *> m_isSelf{};
#else
    static inline thread_local void * sm_isSelf{};
#endif
    mutable XAtomicBool m_occupy{};
#if _LIBCPP_STD_VER >= 20
    mutable std::binary_semaphore m_allow_bin{0};
#else
    mutable Xbinary_Semaphore m_allow_bin{0};
#endif
    mutable std::promise<std::any> m_result{};
    mutable std::weak_ptr<XAbstractTask2> m_next{};
    mutable std::function<bool()> m_is_running{};

    explicit XAbstractTask2Private(Private){}
    ~XAbstractTask2Private() = default;

    static auto create(){
        return std::make_shared<XAbstractTask2Private>(Private{});
    }
};

XAbstractTask2::XAbstractTask2():
m_d_(XAbstractTask2Private::create()){}

bool XAbstractTask2::is_running_() const {
    return m_d_->m_is_running && m_d_->m_is_running();
}

void XAbstractTask2::operator()() {
    try {
#ifdef UNUSE_STD_THREAD_LOCAL
        XThreadLocalRaii<void*> set(m_d_->m_isSelf,this);
#else
        m_d_->sm_isSelf = this;
#endif
        set_result_(run());
    } catch (const std::exception &e) {
        std::cerr << __PRETTY_FUNCTION__ << " exception msg : " << e.what() << "\n";
    }
}

void XAbstractTask2::set_occupy_() const {
    m_d_->m_occupy.storeRelease(true);
}

std::any XAbstractTask2::result_(const Model & m) const {

    constexpr std::string_view selfname{__PRETTY_FUNCTION__};
#if 0
    const XRAII raii{[&selfname]{
        std::cout << selfname << " begin\n" << std::flush;
    },[&selfname]{
        std::cout << selfname << " end\n" << std::flush;
    }};
#endif
    if (NonblockModel == m){
        return m_d_->m_allow_bin.try_acquire() ? m_d_->m_occupy.storeRelease({}),
        m_d_->m_result.get_future().get() : std::any{};
    }

#ifdef UNUSE_STD_THREAD_LOCAL
    if (this == m_d_->m_isSelf.get().value_or(nullptr)) {
#else
    if (this == m_d_->sm_isSelf){
#endif
        std::cerr << selfname << " tips: Working Thread Call invalid\n" << std::flush;
        return {};
    }

    if (!m_d_->m_is_running){
        std::cerr << selfname << " tips: tasks not added\n" << std::flush;
        return {};
    }

    if (!m_d_->m_occupy.loadAcquire()){
        std::cerr << selfname << " tips: Repeated calls\n" << std::flush;
        return {};
    }
    m_d_->m_occupy.storeRelease({});

    m_d_->m_allow_bin.acquire();

    return m_d_->m_result.get_future().get();
}

std::any XAbstractTask2::result_for_(const std::chrono::nanoseconds &rel_time) const {
    constexpr std::string_view selfname{__PRETTY_FUNCTION__};

#if 0
    const XRAII raii{[&selfname]{
        std::cout << selfname << " begin\n" << std::flush;
    },[&selfname]{
        std::cout << selfname << " end\n" << std::flush;
    }};
#endif

    if (!m_d_->m_is_running){
        std::cerr << selfname << " tips: tasks not added\n" << std::flush;
        return {};
    }

    if (!m_d_->m_allow_bin.try_acquire_for(rel_time)){
        std::cerr << selfname << " tips: timeout\n" << std::flush;
        return {};
    }

    return m_d_->m_result.get_future().get();
}

std::any XAbstractTask2::Return_() const{
    return m_d_->m_result.get_future().get();
}

void XAbstractTask2::set_result_(const std::any &v) const {
    m_d_->m_result = {};
    m_d_->m_result.set_value(v);
    m_d_->m_allow_bin.release();
}

void XAbstractTask2::set_exit_function_(std::function<bool()> &&f) const {
    m_d_->m_is_running = std::move(f);
}

[[maybe_unused]] void XAbstractTask2::set_nextHandler(const std::weak_ptr<XAbstractTask2> &next_) {
    m_d_->m_next = next_;
}

[[maybe_unused]] void XAbstractTask2::requestHandler(const std::any &arg) {
    if (const auto p{m_d_->m_next.lock()}){
        p->responseHandler(arg);
    }
}

XAbstractTask2_Ptr XAbstractTask2::joinThreadPool(const XThreadPool2_Ptr & pool) {
    const auto ret{shared_from_this()};
    if (pool){
        pool->taskJoin(ret);
    }
    return ret;
}

#if _LIBCPP_STD_VER >= 20
std::binary_semaphore& XAbstractTask2::operator()(std::nullptr_t) const {
    return m_d_->m_allow_bin;
}
#else
Xbinary_Semaphore &XAbstractTask2::operator()(std::nullptr_t) const {
    return m_d_->m_allow_bin;
}
#endif

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
