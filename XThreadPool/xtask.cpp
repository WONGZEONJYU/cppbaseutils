#include "xtask.hpp"

XTD_NAMESPACE_BEGIN

XTask::XTask(XTask&& obj) noexcept:
XAbstractTask(std::move(obj)){}

XTask & XTask::operator=(XTask &&obj) noexcept {
    XAbstractTask::operator=(std::move(obj));
    return *this;
}

XTD_NAMESPACE_END
