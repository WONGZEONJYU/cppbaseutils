#ifndef XUTILS2_X_CORO_TASK_ABSTRACT_HPP
#define XUTILS2_X_CORO_TASK_ABSTRACT_HPP 1

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
        if (m_coroutine_) {  m_coroutine_.promise().derefCoroutine(); }
        m_coroutine_ = o.m_coroutine_;
        o.m_coroutine_ = {};
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
            using Base::Base;

            constexpr auto await_resume() {
                assert(this->m_awaitedCoroutine_);
                if constexpr (std::is_void_v<T>) {
                    this->m_awaitedCoroutine_.promise().result();
                }else {
                    return std::move(this->m_awaitedCoroutine_.promise().result());
                }
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
    { return thenImplRef(*this, std::forward<ThenCallback>(callback), std::forward<decltype(throwLambda)>(throwLambda)); }

    XCoroTaskAbstractClassTemplate
    template<typename ThenCallback> requires (
        std::is_invocable_v<ThenCallback>
        || (!std::is_void_v<T> && std::is_invocable_v<ThenCallback, T>)
    )
    constexpr auto XCoroTaskAbstractClass then(ThenCallback && callback) &&
    { return thenImpl<XCoroTaskAbstract>(std::move(*this), std::forward<ThenCallback>(callback), std::forward<decltype(throwLambda)>(throwLambda)); }

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
    auto XCoroTaskAbstractClass then(ThenCallback && callback, ErrorCallback &&errorCallback) &&
    { return thenImpl<XCoroTaskAbstract>(std::move(*this), std::forward<ThenCallback>(callback), std::forward<ErrorCallback>(errorCallback)); }

    XCoroTaskAbstractClassTemplate
    template<typename ThenCallback, typename ... Args>
    constexpr auto XCoroTaskAbstractClass invokeCb(ThenCallback && callback, [[maybe_unused]] Args && ... args)
        noexcept(std::is_nothrow_invocable_v<ThenCallback,decltype(std::declval<Args>())...>)
    {
        if constexpr (std::is_invocable_v<ThenCallback,Args...>) {
            return std::invoke(std::forward<ThenCallback>(callback), std::forward<Args>(args)...);
        }else {
            return std::invoke(std::forward<ThenCallback>(callback));
        }
    }

    XCoroTaskAbstractClassTemplate
    template<typename R, typename ErrorCallback , typename U>
    constexpr auto XCoroTaskAbstractClass handleException(ErrorCallback & errCb, std::exception const & exception) -> U
    { errCb(exception); if constexpr (std::is_void_v<R>)  { return ; } else { return U {}; } }

    XCoroTaskAbstractClassTemplate
    template<typename TaskT, typename ThenCallback, typename ErrorCallback, typename R >
    constexpr auto XCoroTaskAbstractClass thenImpl(TaskT task, ThenCallback && thenCallback, ErrorCallback && errorCallback)
        -> std::conditional_t< is_task_v<R>, R, TaskImpl<R> >
    { return thenImplCore(task, thenCallback, errorCallback); }

    XCoroTaskAbstractClassTemplate
    template<typename TaskT, typename ThenCallback, typename ErrorCallback, typename R>
    constexpr auto XCoroTaskAbstractClass thenImplRef(TaskT & task, ThenCallback && thenCallback, ErrorCallback && errorCallback)
        -> std::conditional_t<is_task_v<R>, R, TaskImpl<R>>
    { return thenImplCore(task, thenCallback, errorCallback); }

    XCoroTaskAbstractClassTemplate
    template<typename TaskT, typename ThenCallback, typename ErrorCallback, typename R >
    auto XCoroTaskAbstractClass thenImplCore(TaskT && task_, ThenCallback && thenCallback, ErrorCallback && errorCallback)
        -> std::conditional_t<is_task_v<R>, R, TaskImpl<R>>
    {
        auto thenCb { std::forward<ThenCallback>(thenCallback) };
        auto errCb { std::forward<ErrorCallback>(errorCallback) };
        const auto & task { static_cast<const TaskImpl<T> &>(std::forward<TaskT>(task_)) };

        if constexpr (std::is_void_v<typename TaskImpl<T>::value_type>) {
            try {
                co_await task;
            } catch (std::exception const & e) {
                co_return handleException<R>(errCb, e);
            }
            if constexpr (is_task_v<R>) {
                co_return co_await invokeCb(thenCb);
            } else {
                co_return invokeCb(thenCb);
            }
        } else {
            std::optional<T> value;
            try {
                value.emplace(std::move(co_await task));
            } catch (const std::exception &e) {
                co_return handleException<R>(errCb, e);
            }
            if constexpr (is_task_v<R>) {
                co_return co_await invokeCb(thenCb, std::move(*value));
            } else {
                co_return invokeCb(thenCb, std::move(*value));
            }
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
