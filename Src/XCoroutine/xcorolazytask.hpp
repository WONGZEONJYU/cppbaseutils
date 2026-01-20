#ifndef XUTILS2_X_CORO_LAZY_TASK_HPP
#define XUTILS2_X_CORO_LAZY_TASK_HPP 1

#pragma once

#include <XCoroutine/xcoroutinetask.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template<typename = void> class XCoroLazyTask;

namespace detail {

    template<typename T>
    struct is_task<XCoroLazyTask<T>> : std::true_type
    { using return_type = XCoroLazyTask<T>::value_type; };

    template<typename T>
    struct LazyTaskPromise : TaskPromise<T> {

        using TaskPromise<T>::TaskPromise;

        constexpr XCoroLazyTask<T> get_return_object() noexcept
        { return { std::coroutine_handle<LazyTaskPromise>::from_promise(*this) }; }

        static constexpr auto initial_suspend() noexcept
        { return std::suspend_always {}; }
    };

}

template<typename T>
class XCoroLazyTask final
    : public detail::XCoroTaskAbstract<T, XCoroLazyTask, detail::LazyTaskPromise<T>>
{
    using Base = detail::XCoroTaskAbstract<T, XCoroLazyTask, detail::LazyTaskPromise<T>>;
public:
    using promise_type = detail::LazyTaskPromise<T>;
    using value_type = T;

    ~XCoroLazyTask() override;

    auto operator co_await() const noexcept;

    constexpr XCoroLazyTask() noexcept = default;

    explicit(false) constexpr XCoroLazyTask(Base::coroutine_handle const h)
        : Base { h } {}
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#define X_COROUTINE_
#include <XCoroutine/impl/corolazytask.hpp>
#undef X_COROUTINE_

#endif
