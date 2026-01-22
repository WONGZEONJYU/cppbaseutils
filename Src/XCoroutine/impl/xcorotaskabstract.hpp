#ifndef XUTILS2_X_CORO_TASK_ABSTRACT_HPP
#define XUTILS2_X_CORO_TASK_ABSTRACT_HPP 1

#ifndef X_COROUTINE_
#error Do not xcorotaskabstract.hpp directly
#endif

#pragma once

#include <cassert>
#include <optional>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

#undef XCoroTaskAbstractClassTemplate
#define XCoroTaskAbstractClassTemplate template< typename T, template<typename> class TaskImpl, typename PromiseType >

#undef XCoroTaskAbstractClass
#define XCoroTaskAbstractClass XCoroTaskAbstract<T,TaskImpl,PromiseType>::

    XCoroTaskAbstractClassTemplate
    constexpr auto XCoroTaskAbstractClass operator=(XCoroTaskAbstract && o) noexcept
        -> XCoroTaskAbstract &
    {
#if 0
        // if (this == std::addressof(o)) { return *this; }
        // if (m_coroutine_) { m_coroutine_.promise().derefCoroutine(); }
        // m_coroutine_ = o.m_coroutine_;
        // o.m_coroutine_ = {};
#else
        XCoroTaskAbstract { std::move(o) }.swap(*this);
        return *this;
#endif
    }

    XCoroTaskAbstractClassTemplate
    auto XCoroTaskAbstractClass operator co_await() const noexcept {

        struct TaskAwaiter final : TaskAwaiterAbstract<PromiseType> {

            explicit(false) constexpr TaskAwaiter(coroutine_handle const h)
                : TaskAwaiterAbstract<PromiseType> { h }
            {    }

            constexpr auto await_resume() const {
                assert(this->m_awaitedCoroutine_);
                if constexpr (std::is_void_v<T>) {
                    this->m_awaitedCoroutine_.promise().result();
                } else {
                    return std::move(this->m_awaitedCoroutine_.promise().result());
                }
            }
        };

        return TaskAwaiter { m_coroutine_ };
    }

    XCoroTaskAbstractClassTemplate
    template<typename ThenCallback, typename ... Args>
    constexpr auto XCoroTaskAbstractClass invokeCb(ThenCallback && callback, Args && ... args)
        noexcept(std::is_nothrow_invocable_v<ThenCallback,Args...>)
    {
        if constexpr (std::is_invocable_v<ThenCallback,Args...>)
        { return std::invoke(std::forward<ThenCallback>(callback), std::forward<Args>(args)...); }
        else { return std::invoke(std::forward<ThenCallback>(callback)); }
    }

    XCoroTaskAbstractClassTemplate
    template<typename TaskT, typename ThenCallback, typename ErrorCallback, typename R >
    auto XCoroTaskAbstractClass thenImpl(TaskT const task, ThenCallback && thenCallback, ErrorCallback && errorCallback)
        -> std::conditional_t< is_task_v<R>, R, TaskImpl<R> >
    {
        auto thenCb { std::forward<ThenCallback>(thenCallback) };
        auto errCb { std::forward<ErrorCallback>(errorCallback) };
        auto && taskImpl { static_cast< TaskImpl<T> const & >(task) };

        if constexpr (std::is_void_v<typename TaskImpl<T>::value_type>) {
            try { co_await taskImpl; }
            catch (std::exception const & e) { co_return handleException<R>(errCb, e); }

            if constexpr (is_task_v<R>) { co_return co_await invokeCb(thenCb); }
            else { co_return invokeCb(thenCb); }
        } else {
            std::optional<T> value {};

            try { value.emplace(std::move(co_await taskImpl)); }
            catch (std::exception const & e) { co_return handleException<R>(errCb, e); }

            if constexpr (is_task_v<R>) { co_return co_await invokeCb(thenCb , std::move(*value)); }
            else { co_return invokeCb(thenCb , std::move(*value)); }
        }
    }

    XCoroTaskAbstractClassTemplate
    template<typename TaskT, typename ThenCallback, typename ErrorCallback, typename R>
    auto XCoroTaskAbstractClass thenImplRef(TaskT const & task, ThenCallback && thenCallback, ErrorCallback && errorCallback)
        -> std::conditional_t<is_task_v<R>, R, TaskImpl<R>>
    {
        auto thenCb { std::forward<ThenCallback>(thenCallback) };
        auto errCb { std::forward<ErrorCallback>(errorCallback) };
        auto && taskImpl { static_cast< TaskImpl<T> const & >(task) };

        if constexpr (std::is_void_v<typename TaskImpl<T>::value_type>) {
            try { co_await taskImpl; }
            catch (std::exception const & e) { co_return handleException<R>(errCb, e); }

            if constexpr (is_task_v<R>) { co_return co_await invokeCb(thenCb); }
            else { co_return invokeCb(thenCb); }
        } else {
            std::optional<T> value {};

            try { value.emplace(std::move(co_await taskImpl)); }
            catch (std::exception const & e) { co_return handleException<R>(errCb, e); }

            if constexpr (is_task_v<R>) { co_return co_await invokeCb(thenCb , std::move(*value)); }
            else { co_return invokeCb(thenCb , std::move(*value)); }
        }
    }

#undef XCoroTaskAbstractClass
#undef XCoroTaskAbstractClassTemplate

}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
