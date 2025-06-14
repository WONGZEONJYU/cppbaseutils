#include "xabstracttask2.hpp"
#include <iostream>
#include <semaphore>
#include <future>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XAbstractTask2::XAbstractTask2Private {
    friend class XAbstractTask2;
    enum class Private{};
    std::binary_semaphore m_semaphore_{0};
    std::promise<std::any> m_promise_{};
public:
    explicit XAbstractTask2Private(Private){}
    static auto create(){
        return std::make_shared<XAbstractTask2Private>(Private{});
    }
};

XAbstractTask2::XAbstractTask2():
m_d_(XAbstractTask2Private::create()){
}

bool XAbstractTask2::is_running_() const
{
    return m_d_.get();
}

void XAbstractTask2::exec_(){
    set_result_(run());
}

std::any XAbstractTask2::result_() const{
    m_d_->m_semaphore_.acquire();
    return m_d_->m_promise_.get_future().get();
}

void XAbstractTask2::set_result_(const std::any &v) const{
    m_d_->m_promise_= {};
    m_d_->m_promise_.set_value(v);
    m_d_->m_semaphore_.release();
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
