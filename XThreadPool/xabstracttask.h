#pragma once
#ifndef X_ABSTRACT_TASK_HPP
#define X_ABSTRACT_TASK_HPP 1

#include <functional>
#include <future>

namespace xtd {
    class XAbstractTask {
        void swap(XAbstractTask &) noexcept;
    protected:
        using exit_t = std::function<bool()>;

        void set_return_(const int64_t &v) {
            m_return_.set_value(v);
        }

        void set_exit_(exit_t &&e){
            m_is_exit_ = e;
        }

        auto get_return(){
            return m_return_.get_future().get();
        }
    private:
        exit_t m_is_exit_{};
        std::promise<int64_t> m_return_{};
    protected:
        XAbstractTask() = default;
        XAbstractTask(XAbstractTask &&) noexcept;
        XAbstractTask& operator=(XAbstractTask && )noexcept;
        virtual ~XAbstractTask() = default;
    public:
        XAbstractTask(const XAbstractTask&) = delete;
        XAbstractTask& operator=(const XAbstractTask&) = delete;
    };
}

#endif
