#pragma once
#ifndef X_TASK_HPP
#define X_TASK_HPP 1

#include "xabstracttask.h"

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XTask : public XAbstractTask {
	X_DISABLE_COPY(XTask)
	friend class XThreadPool;
	virtual int64_t run() {return 0;}
	[[nodiscard]][[maybe_unused]] virtual int64_t run() const {return 0;}
protected:
    XTask() = default;
public:
    using XAbstractTask::get_return;
	XTask(XTask &&) noexcept;
	XTask &operator=(XTask &&) noexcept;
	~XTask() override = default;
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
