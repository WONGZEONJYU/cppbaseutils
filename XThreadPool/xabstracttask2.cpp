#include "xabstracttask2.hpp"
#include "xthreadpool2.hpp"
#include <future>
#include <iostream>

#define USE_ATOMIC 1

#if USE_ATOMIC
#include <XAtomic/xatomic.hpp>
#else
#include <semaphore>
#endif

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XAbstractTask2::XAbstractTask2Private final {
    enum class Private{};
public:
    static inline thread_local bool sm_isWorkThread{};
    static inline thread_local void * sm_isSelf{};
    XAtomicBool m_occupy{};
#if USE_ATOMIC
    XAtomicBool m_allow{};
#else
    std::binary_semaphore m_allow{0};
#endif
    std::promise<std::any> m_result{};
    std::weak_ptr<XAbstractTask2> m_next{};
    std::function<bool()> m_is_running{};

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
#if 0
        m_d_->sm_isWorkThread = true;
        set_result_(run());
        m_d_->sm_isWorkThread = false;
#else
        m_d_->sm_isSelf = this;
        set_result_(run());
#endif
    } catch (const std::exception &e) {
        std::cerr << __PRETTY_FUNCTION__ << " exception msg : " << e.what() << "\n";
    }
}

void XAbstractTask2::set_occupy_() const{
    m_d_->m_occupy.storeRelease(true);
}

std::any XAbstractTask2::result_() const {

    constexpr std::string_view selfname{__PRETTY_FUNCTION__};
    const XRAII raii{[&selfname]{
        std::cout << selfname << " begin\n" << std::flush;
    },[&selfname]{
        std::cout << selfname << " end\n" << std::flush;
    }};

    if (this == m_d_->sm_isSelf){
        std::cerr << selfname << " tips: Working Thread Call invalid\n" << std::flush;
        return {};
    }

    if (!m_d_->m_is_running || !m_d_->m_occupy.loadAcquire()){
        std::cerr << selfname << " tips: Repeated calls or tasks not added\n" << std::flush;
        return {};
    }
    m_d_->m_occupy.storeRelease({});
#if USE_ATOMIC
    m_d_->m_allow.m_x_value.wait({},std::memory_order_acquire);
    m_d_->m_allow.storeRelease({});
#else
    m_d_->m_allow.acquire();
#endif
    return m_d_->m_result.get_future().get();
}

void XAbstractTask2::set_result_(const std::any &v) const {
    m_d_->m_result = {};
    m_d_->m_result.set_value(v);
#if USE_ATOMIC
    m_d_->m_allow.storeRelease(true);
    m_d_->m_allow.m_x_value.notify_all();
#else
    m_d_->m_allow.release();
#endif
}

void XAbstractTask2::set_exit_function_(std::function<bool()> &&f) const {
    m_d_->m_is_running = std::move(f);
}

void XAbstractTask2::set_nextHandler(const std::weak_ptr<XAbstractTask2> &next_) {
    m_d_->m_next = next_;
}

void XAbstractTask2::requestHandler(const std::any &arg) {
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

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
