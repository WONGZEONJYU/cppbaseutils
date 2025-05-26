#pragma once
#ifndef X_TASK_HPP
#define X_TASK_HPP 1

#include "xabstracttask.h"

namespace xtd {
    class XTask : public XAbstractTask {

		virtual int64_t Run() {return 0;}
		[[nodiscard]][[maybe_unused]] virtual int64_t Run() const {return 0;}

    protected:
        explicit XTask() = default;
	public:
        using XAbstractTask::get_return;
	public:
		XTask(const XTask&) = delete;
		XTask& operator=(const XTask&) = delete;

		XTask(XTask &&) noexcept;
		XTask &operator=(XTask &&) noexcept;

		~XTask() override = default;

		friend class XThreadPool;
	};
}

#endif
