#include "xabstracttask2.hpp"
#include <semaphore>
#include <future>
#include <iostream>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XAbstractTask2::XAbstractTask2Private final {
    enum class Private{};
public:
    explicit XAbstractTask2Private(Private){}
    ~XAbstractTask2Private() = default;
    static auto create(){
        return std::make_shared<XAbstractTask2Private>(Private{});
    }
    std::binary_semaphore m_semaphore_{0};
    std::promise<std::any> m_result_{};
    std::weak_ptr<XAbstractTask2> m_next_{};
    std::function<bool()> m_is_running_{};
};

XAbstractTask2::XAbstractTask2():
m_d_(XAbstractTask2Private::create()){}

[[maybe_unused]] bool XAbstractTask2::is_running_() const{
    return m_d_->m_is_running_ && m_d_->m_is_running_();
}

void XAbstractTask2::exec_() {
    try {
        set_result_(run());
    } catch (const std::exception &e) {
        std::cerr << __PRETTY_FUNCTION__ << " exception msg : " << e.what() << "\n";
    }
}

std::any XAbstractTask2::result_() const{
    m_d_->m_semaphore_.acquire();
    return m_d_->m_result_.get_future().get();
}

void XAbstractTask2::set_result_(const std::any &v) const{
    m_d_->m_result_= {};
    m_d_->m_result_.set_value(v);
    m_d_->m_semaphore_.release();
}

void XAbstractTask2::set_exit_function_(std::function<bool()> &&f) const {
    m_d_->m_is_running_ = std::move(f);
}

[[maybe_unused]] void XAbstractTask2::set_nextHandler(const std::weak_ptr<XAbstractTask2> &next_) {
    m_d_->m_next_ = next_;
}

[[maybe_unused]] void XAbstractTask2::requestHandler(const std::any &arg) {
    if (const auto p{m_d_->m_next_.lock()}){
        p->responseHandler(arg);
    }
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
