#include "xabstracttask2.hpp"

#include <iostream>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

void XAbstractTask2::exec(){
    m_promise_.set_value(run());
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
