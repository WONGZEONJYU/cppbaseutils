#include "xtask.hpp"

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

XTask::XTask(XTask&& obj) noexcept:
XAbstractTask(std::move(obj)){}

XTask & XTask::operator=(XTask &&obj) noexcept {
    XAbstractTask::operator=(std::move(obj));
    return *this;
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
