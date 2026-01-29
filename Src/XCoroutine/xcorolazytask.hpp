#ifndef XUTILS2_X_CORO_LAZY_TASK_HPP
#define XUTILS2_X_CORO_LAZY_TASK_HPP 1

#pragma once

#include <XCoroutine/xcoroutinetask.hpp>
#include <iostream>
#include <cassert>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template<typename T> class XCoroLazyTask;

namespace detail {

    template<typename T>
    struct is_task<XCoroLazyTask<T>> : std::true_type
    { using return_type = XCoroLazyTask<T>::value_type; };

    template<typename T>
    struct LazyTaskPromise : TaskPromise<T> {

        using TaskPromise<T>::TaskPromise;

        constexpr XCoroLazyTask<T> get_return_object() noexcept
        { return { this }; }

        static constexpr auto initial_suspend() noexcept
        { return std::suspend_always {}; }
    };

}

template<typename T = void>
class XCoroLazyTask final
    : public detail::XCoroTaskAbstract<T, XCoroLazyTask, detail::LazyTaskPromise<T>>
{
    using Base = detail::XCoroTaskAbstract<T, XCoroLazyTask, detail::LazyTaskPromise<T>>;
    using coroutine_handle = Base::coroutine_handle;

public:
    using promise_type = detail::LazyTaskPromise<T>;
    using value_type = T;

    ~XCoroLazyTask() override {
#ifndef NDEBUG
        if (this->m_coroutine_ && !this->m_coroutine_.done())
        { std::cerr << "XCoroLazyTask destroyed before it was awaited!"; }
#endif
    }

    constexpr auto operator co_await() const noexcept {

        struct TaskAwaiter : detail::TaskAwaiterAbstract<promise_type> {
            using Base = detail::TaskAwaiterAbstract<promise_type>;

            X_IMPLICIT constexpr TaskAwaiter(Base::coroutine_handle const h) noexcept
                : Base { h } {  }

            constexpr auto await_suspend(std::coroutine_handle<> const h) noexcept{
                Base::await_suspend(h);
                // Return handle to the lazy task, so that it gets automatically resumed.
                return this->m_awaitedCoroutine_;
            }

            constexpr auto await_resume(){
                assert(this->m_awaitedCoroutine_);
                if constexpr (std::is_void_v<T>) { this->m_awaitedCoroutine_.promise().result(); }
                else { return std::move(this->m_awaitedCoroutine_.promise().result()); }
            }
        };

        return TaskAwaiter { this->m_coroutine_ };
    }

    constexpr XCoroLazyTask() noexcept = default;

    X_IMPLICIT constexpr XCoroLazyTask(coroutine_handle const h)
        : Base { h }
    {   }

    X_IMPLICIT constexpr XCoroLazyTask(promise_type & promise)
        : XCoroLazyTask { coroutine_handle::from_promise(promise) }
    {   }

    X_IMPLICIT constexpr XCoroLazyTask(promise_type * const promise)
        : XCoroLazyTask { *promise }
    {   }
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
