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
        set_result_(run());
#endif
    } catch (const std::exception &e) {
        std::cerr << __PRETTY_FUNCTION__ << " exception msg : " << e.what() << "\n";
    }
}

std::any XAbstractTask2::result_() const {
#if 0
    if (m_d_->sm_isWorkThread){
        std::cerr << __PRETTY_FUNCTION__ << " tips: WorkThread call invalid\n" << std::flush;
        return {};
    }
#endif

    std::cout << __PRETTY_FUNCTION__ << " begin\n" << std::flush;
#if USE_ATOMIC
    m_d_->m_allow.m_x_value.wait({},std::memory_order_acquire);
    m_d_->m_allow.storeRelease({});
#else
    m_d_->m_allow.acquire();
#endif
    std::cout << __PRETTY_FUNCTION__ << " end\n" << std::flush;
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
