#ifndef XUTILS2_X_CORO_MANAGER_HPP_
#define XUTILS2_X_CORO_MANAGER_HPP_ 1

#pragma once

#include <XGlobal/xversion.hpp>
#include <XGlobal/xclasshelpermacros.hpp>
#include <coroutine>
#include <memory>
#include <functional>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {
    template<typename T> class TaskPromise;

    class TaskFinalSuspend;

    template<typename T> struct LazyTaskPromise;

    using cb_t = std::function<void()>;
}

class XCoroManagerPrivate;

class X_CLASS_EXPORT XCoroManager final {

    friend class detail::TaskFinalSuspend;

    template<typename T>
    friend class detail::TaskPromise;

    template<typename T>
    friend struct detail::LazyTaskPromise;

    std::unique_ptr<XCoroManagerPrivate> m_d_ptr_;
    X_DECLARE_PRIVATE_D(m_d_ptr_,XCoroManager)

public:
    [[nodiscard]] std::size_t onlineSize() const noexcept;
    void setAllExitCallback(detail::cb_t &&) const noexcept;

private:
    XCoroManager();
    [[nodiscard]] bool add(std::coroutine_handle<> const & h) const noexcept ;
    void remove(std::coroutine_handle<> const &) const noexcept;

public:
    static XCoroManager & instance();
    ~XCoroManager();
    X_DISABLE_COPY_MOVE(XCoroManager)
};

X_API XCoroManager * coroMgrPtr() noexcept;
X_API XCoroManager & coroMgrRef() noexcept;

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
