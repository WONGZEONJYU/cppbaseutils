#include "xabstracttask2.hpp"

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

void XAbstractTask2::exec(){
    std::packaged_task task([this]{ return run(); });
    m_result_ = task.get_future();
    task();
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
