#pragma once
#ifndef X_ABSTRACT_TASK_HPP
#define X_ABSTRACT_TASK_HPP 1

#include <functional>
#include <future>
#include "../XHelper/xhelper.hpp"

XTD_NAMESPACE_BEGIN

class XAbstractTask {
    X_DISABLE_COPY(XAbstractTask)
    void swap(XAbstractTask &) noexcept;
protected:
    using exit_t = std::function<bool()>;

    void set_return_(const int64_t &v) {
        m_return_.set_value(v);
    }

    void set_exit_cnd(exit_t &&e){
        m_is_exit_ = e;
    }

    auto get_return(){
        return m_return_.get_future().get();
    }

    [[nodiscard]] auto is_exit_()const{return m_is_exit_();}

private:
    exit_t m_is_exit_{};
    std::promise<int64_t> m_return_{};
protected:
    XAbstractTask() = default;
    XAbstractTask(XAbstractTask &&) noexcept;
    XAbstractTask& operator=(XAbstractTask && )noexcept;
    virtual ~XAbstractTask() = default;
};

XTD_NAMESPACE_END

#endif
