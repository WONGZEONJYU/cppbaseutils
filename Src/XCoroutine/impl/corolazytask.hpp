#ifndef XUTILS2_CORO_LAZY_TASK_HPP
#define XUTILS2_CORO_LAZY_TASK_HPP 1

#ifndef X_COROUTINE_
#error Do not corolazytask.hpp directly
#endif

#pragma once

#include <iostream>
#include <cassert>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    template <typename T>
    constexpr XCoroLazyTask<T> LazyTaskPromise<T>::get_return_object() noexcept
    { return XCoroLazyTask<T> { std::coroutine_handle<LazyTaskPromise>::from_promise(*this) }; }

    template <typename T>
    constexpr std::suspend_always LazyTaskPromise<T>::initial_suspend() noexcept
    { return {}; }

}

template<typename T>
XCoroLazyTask<T>::~XCoroLazyTask() {
    if (this->m_coroutine_ && !this->m_coroutine_.done())
    { std::cerr << "XCoroLazyTask destroyed before it was awaited!"; }
}

template<typename T>
auto XCoroLazyTask<T>::operator co_await() const noexcept{

    struct TaskAwaiter final: detail::TaskAwaiterAbstract<promise_type> {
        using Base = detail::TaskAwaiterAbstract<promise_type>;

        explicit(false) constexpr TaskAwaiter(Base::coroutine_handle const h) noexcept
            : Base { h } {  }

        constexpr auto await_suspend(std::coroutine_handle<> const awaitingCoroutine) noexcept{
            Base::await_suspend(awaitingCoroutine);
            // Return handle to the lazy task, so that it gets automatically resumed.
            return this->m_awaitedCoroutine_;
        }

        constexpr auto await_resume(){
            assert(this->m_awaitedCoroutine_);
            if constexpr (std::is_void_v<T>) { this->mAwaitedCoroutine.promise().result(); }
            else { return this->m_awaitedCoroutine_.promise().result(); }
        }
    };

    return TaskAwaiter { this->m_coroutine_ };
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
