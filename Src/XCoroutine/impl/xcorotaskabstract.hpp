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
# define XCoroTaskAbstractClassTemplate \
    template< typename T, template<typename> class TaskImpl, typename PromiseType >

#undef XCoroTaskAbstractClass
#define XCoroTaskAbstractClass \
    XCoroTaskAbstract<T,TaskImpl,PromiseType>::

    XCoroTaskAbstractClassTemplate
    constexpr XCoroTaskAbstractClass XCoroTaskAbstract(XCoroTaskAbstract && o) noexcept
    { swap(o); }

    XCoroTaskAbstractClassTemplate
    constexpr auto XCoroTaskAbstractClass operator=(XCoroTaskAbstract && o) noexcept
        -> XCoroTaskAbstract &
    { swap(o); return *this; }

    XCoroTaskAbstractClassTemplate
    constexpr void XCoroTaskAbstractClass swap(XCoroTaskAbstract & o) noexcept {
        if (this == std::addressof(o)) { return; }
        std::swap(m_coroutine_, o.m_coroutine_);
    }

    XCoroTaskAbstractClassTemplate
    XCoroTaskAbstractClass ~XCoroTaskAbstract()
    { if (m_coroutine_) { m_coroutine_.promise().derefCoroutine(); } }

    XCoroTaskAbstractClassTemplate
    constexpr bool XCoroTaskAbstractClass isReady() const
    { return !m_coroutine_ || m_coroutine_.done(); }

    XCoroTaskAbstractClassTemplate
    auto XCoroTaskAbstractClass operator co_await() const noexcept{

        struct TaskAwaiter final : TaskAwaiterAbstract<PromiseType> {

            using Base = TaskAwaiterAbstract<PromiseType>;

            explicit(false) constexpr TaskAwaiter(Base::coroutine_handle const h) : Base {h} {}

            constexpr auto await_resume() {
                assert(this->m_awaitedCoroutine_);
                if constexpr (std::is_void_v<T>) { this->m_awaitedCoroutine_.promise().result(); }
                else { return this->m_awaitedCoroutine_.promise().result(); }
            }
        };

        return TaskAwaiter { m_coroutine_ };
    }

    inline auto throwLambda { []<typename T>(T && ){ throw; } };

    XCoroTaskAbstractClassTemplate
    template<typename ThenCallback> requires (
        std::is_invocable_v<ThenCallback>
        || (!std::is_void_v<T> && std::is_invocable_v<ThenCallback, T>)
    )
    constexpr auto XCoroTaskAbstractClass then(ThenCallback && callback) &
    { return thenImplRef(*this, std::forward<ThenCallback>(callback),throwLambda); }

    XCoroTaskAbstractClassTemplate
    template<typename ThenCallback> requires (
        std::is_invocable_v<ThenCallback>
        || (!std::is_void_v<T> && std::is_invocable_v<ThenCallback, T>)
    )
    constexpr auto XCoroTaskAbstractClass then(ThenCallback && callback) &&
    { return thenImpl<XCoroTaskAbstract>(std::move(*this), std::forward<ThenCallback>(callback), throwLambda); }

    XCoroTaskAbstractClassTemplate
    template<typename ThenCallback, typename ErrorCallback>
    requires (
        ( std::is_invocable_v<ThenCallback> || (!std::is_void_v<T> && std::is_invocable_v<ThenCallback, T>) )
        && std::is_invocable_v<ErrorCallback, std::exception const &>
    )
    auto XCoroTaskAbstractClass then(ThenCallback && callback, ErrorCallback && errorCallback) &
    { return thenImplRef(*this, std::forward<ThenCallback>(callback), std::forward<ErrorCallback>(errorCallback)); }

    XCoroTaskAbstractClassTemplate
    template<typename ThenCallback, typename ErrorCallback>
    requires (
        ( std::is_invocable_v<ThenCallback> || (!std::is_void_v<T> && std::is_invocable_v<ThenCallback, T>) )
        && std::is_invocable_v<ErrorCallback, std::exception const &>
    )
    auto XCoroTaskAbstractClass then(ThenCallback && callback, ErrorCallback && errorCallback) &&
    { return thenImpl<XCoroTaskAbstract>(std::move(*this), std::forward<ThenCallback>(callback), std::forward<ErrorCallback>(errorCallback)); }

    XCoroTaskAbstractClassTemplate
    template<typename ThenCallback, typename ... Args>
    constexpr auto XCoroTaskAbstractClass invokeCb(ThenCallback && callback, [[maybe_unused]] Args && ... args)
        noexcept(std::is_nothrow_invocable_v<ThenCallback,decltype(std::declval<Args>())...>)
    {
        if constexpr (std::is_invocable_v<ThenCallback,Args...>)
        { return std::invoke(std::forward<ThenCallback>(callback), std::forward<Args>(args)...); }
        else { return std::invoke(std::forward<ThenCallback>(callback)); }
    }

    XCoroTaskAbstractClassTemplate
    template<typename R, typename ErrorCallback , typename U>
    constexpr U XCoroTaskAbstractClass handleException(ErrorCallback && errCb, std::exception const & exception)
    { std::invoke(std::forward<ErrorCallback>(errCb),exception); if constexpr (std::is_void_v<R>) { return ; } else { return U{}; } }

    XCoroTaskAbstractClassTemplate
    template<typename TaskT, typename ThenCallback, typename ErrorCallback, typename R>
    constexpr auto XCoroTaskAbstractClass thenImpl(TaskT task, ThenCallback && thenCallback, ErrorCallback && errorCallback)
        -> std::conditional_t< is_task_v<R>, R, TaskImpl<R> >
    { return thenImplCore(std::move(task), std::forward<ThenCallback>(thenCallback), std::forward<ErrorCallback>(errorCallback)); }

    XCoroTaskAbstractClassTemplate
    template<typename TaskT, typename ThenCallback, typename ErrorCallback, typename R>
    constexpr auto XCoroTaskAbstractClass thenImplRef(TaskT & task, ThenCallback && thenCallback, ErrorCallback && errorCallback)
        -> std::conditional_t<is_task_v<R>, R, TaskImpl<R>>
    { return thenImplCore(task, std::forward<ThenCallback>(thenCallback), std::forward<ErrorCallback>(errorCallback)); }

    XCoroTaskAbstractClassTemplate
    template<typename TaskT, typename ThenCallback, typename ErrorCallback, typename R>
    auto XCoroTaskAbstractClass thenImplCore(TaskT && task_, ThenCallback && thenCallback, ErrorCallback && errorCallback)
        -> std::conditional_t<is_task_v<R>, R, TaskImpl<R>>
    {

        auto && task__ { std::forward<TaskT>(task_) };
        auto && task { static_cast< TaskImpl<T> const & >( task__ ) };

        if constexpr (std::is_void_v<typename TaskImpl<T>::value_type>) {
            try { co_await task;}
            catch (std::exception const & e) { co_return handleException<R>(std::forward<ErrorCallback>(errorCallback), e); }

            if constexpr (is_task_v<R>) { co_return co_await invokeCb(std::forward<ThenCallback>(thenCallback)); }
            else { co_return invokeCb(std::forward<ThenCallback>(thenCallback)); }
        } else {
            std::optional<T> value {};

            try { value.emplace(std::move(co_await task)); }
            catch (std::exception const & e) { co_return handleException<R>(std::forward<ErrorCallback>(errorCallback), e); }

            if constexpr (is_task_v<R>) { co_return co_await invokeCb(std::forward<ThenCallback>(thenCallback) , std::move(*value)); }
            else { co_return invokeCb(std::forward<ThenCallback>(thenCallback) , std::move(*value)); }
        }
    }

    XCoroTaskAbstractClassTemplate
    constexpr XCoroTaskAbstractClass XCoroTaskAbstract(std::coroutine_handle<PromiseType> const coroutine) noexcept
        : m_coroutine_ { coroutine }
    { m_coroutine_.promise().refCoroutine(); }

#undef XCoroTaskAbstractClass
#undef XCoroTaskAbstractClassTemplate

}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
