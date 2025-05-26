#include "xtask.hpp"

namespace xtd {

    XTask::XTask(XTask&& obj) noexcept:
    XAbstractTask(std::move(obj)){

    }

    XTask & XTask::operator=(XTask &&obj) noexcept {
        XAbstractTask::operator=(std::move(obj));
        return *this;
    }
}
