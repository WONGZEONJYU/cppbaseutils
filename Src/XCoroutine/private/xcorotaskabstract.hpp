#ifndef XUTILS2_X_CORO_TASK_ABSTRACT_HPP
#define XUTILS2_X_CORO_TASK_ABSTRACT_HPP 1

#ifndef X_COROUTINE_
#error Do not xcorotaskabstract.hpp directly
#endif

#pragma once

#include <XGlobal/xclasshelpermacros.hpp>
#include <XCoroutine/private/taskawaiterabstract.hpp>
#include <cassert>
#include <optional>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    template<typename T>
    struct is_task : std::false_type { using return_type = T; };

    template<typename T>
    struct is_task<XCoroTask<T>> : std::true_type
    { using return_type = XCoroTask<T>::value_type; };

    template<typename T>
    inline constexpr auto is_task_v { is_task<T>::value };

    template<typename T>
    using is_task_rt = is_task<T>::return_type;

    template<typename T, template<typename> class TaskImpl, typename PromiseType>
    class XCoroTaskAbstract {
        static auto constexpr ErrCallBack { []<typename Tp>(Tp && ){ throw; } };
    protected:
        using coroutine_handle = std::coroutine_handle<PromiseType>;
        coroutine_handle m_coroutine_ {};

    public:
        constexpr XCoroTaskAbstract(XCoroTaskAbstract && o) noexcept
            : m_coroutine_ { o.m_coroutine_ }
        { o.m_coroutine_ = {}; }

        constexpr XCoroTaskAbstract & operator=(XCoroTaskAbstract && o) noexcept {
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

        virtual ~XCoroTaskAbstract()
        { if (m_coroutine_) { m_coroutine_.promise().derefCoroutine(); } }

        [[nodiscard]] constexpr bool isReady() const
        { return !m_coroutine_ || m_coroutine_.done(); }

        auto operator co_await() const noexcept;

        constexpr void swap(XCoroTaskAbstract & o) noexcept
        { std::swap(m_coroutine_, o.m_coroutine_); }

        template<typename ThenCallback> requires (
            std::is_invocable_v<ThenCallback>
            || (!std::is_void_v<T> && std::is_invocable_v<ThenCallback, T>)
        )
        constexpr auto then(ThenCallback && callback) &
        { return thenImplRef(*this, std::forward<ThenCallback>(callback),ErrCallBack); }

        template<typename ThenCallback> requires (
            std::is_invocable_v<ThenCallback>
            || (!std::is_void_v<T> && std::is_invocable_v<ThenCallback, T>)
        )
        constexpr auto then(ThenCallback && callback) &&
        { return thenImpl(std::move(*this), std::forward<ThenCallback>(callback), ErrCallBack); }

        template<typename ThenCallback, typename ErrorCallback> requires (
            ( std::is_invocable_v<ThenCallback> || (!std::is_void_v<T> && std::is_invocable_v<ThenCallback, T>) )
            && std::is_invocable_v<ErrorCallback, std::exception const &>
        )
        constexpr auto then(ThenCallback && callback, ErrorCallback && errorCallback) &
        { return thenImplRef(*this, std::forward<ThenCallback>(callback), std::forward<ErrorCallback>(errorCallback)); }

        template<typename ThenCallback, typename ErrorCallback> requires (
            ( std::is_invocable_v<ThenCallback> || (!std::is_void_v<T> && std::is_invocable_v<ThenCallback, T>) )
            && std::is_invocable_v<ErrorCallback, std::exception const &>
        )
        constexpr auto then(ThenCallback && callback, ErrorCallback && errorCallback) &&
        { return thenImpl(std::move(*this), std::forward<ThenCallback>(callback), std::forward<ErrorCallback>(errorCallback)); }

    private:
        template<typename ThenCallback, typename ... Args>
        static constexpr auto invokeCb(ThenCallback && callback,[[maybe_unused]] Args && ... args)
            noexcept(std::is_nothrow_invocable_v<ThenCallback,Args...>)
        {
            if constexpr (std::is_invocable_v<ThenCallback,Args...>)
            { return std::invoke(std::forward<ThenCallback>(callback), std::forward<Args>(args)...); }
            else { return std::invoke(std::forward<ThenCallback>(callback)); }
        }

        template<typename R, typename ErrorCallback , typename U = is_task_rt<R>>
        static constexpr U handleException(ErrorCallback && errCb, std::exception const & exception)
        { std::invoke(std::forward<ErrorCallback>(errCb),exception); if constexpr (std::is_void_v<U>) { return ; } else { return U{}; } }

        template<typename ThenCallback, typename ...Arg>
        struct cb_invoke_result : std::conditional_t<
            std::is_invocable_v<ThenCallback>,
                std::invoke_result<ThenCallback>,
                std::invoke_result<ThenCallback, Arg...>
            >
        {   };

        template<typename ThenCallback>
        struct cb_invoke_result<ThenCallback, void> : std::invoke_result<ThenCallback> {};

        template<typename ThenCallback, typename ...Arg>
        using cb_invoke_result_t = cb_invoke_result<ThenCallback, Arg...>::type;

        template<typename TaskT, typename ThenCallback, typename ErrorCallback, typename R = cb_invoke_result_t<ThenCallback, T>>
        static auto thenImpl(TaskT, ThenCallback && , ErrorCallback && )
            -> std::conditional_t< is_task_v<R>, R, TaskImpl<R> >;

        template<typename TaskT, typename ThenCallback, typename ErrorCallback, typename R = cb_invoke_result_t<ThenCallback, T>>
        static auto thenImplRef(TaskT const &, ThenCallback && , ErrorCallback && )
            -> std::conditional_t<is_task_v<R>, R, TaskImpl<R>>;

    protected:
        constexpr XCoroTaskAbstract() noexcept = default;

        X_IMPLICIT constexpr XCoroTaskAbstract(coroutine_handle const h) noexcept
            : m_coroutine_ { h }
        { m_coroutine_.promise().refCoroutine(); }

        X_DISABLE_COPY(XCoroTaskAbstract)
    };

#undef XCoroTaskAbstractClassTemplate
#undef XCoroTaskAbstractClass
#define XCoroTaskAbstractClassTemplate template< typename T, template<typename> class TaskImpl, typename PromiseType>
#define XCoroTaskAbstractClass XCoroTaskAbstract<T,TaskImpl,PromiseType>::

    XCoroTaskAbstractClassTemplate
    auto XCoroTaskAbstractClass operator co_await() const noexcept {

        struct TaskAwaiter final : TaskAwaiterAbstract<PromiseType> {

            X_IMPLICIT constexpr TaskAwaiter(coroutine_handle const h)
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
    template<typename TaskT, typename ThenCallback, typename ErrorCallback, typename R >
    auto XCoroTaskAbstractClass thenImpl(TaskT const task, ThenCallback && thenCallback, ErrorCallback && errorCallback)
        -> std::conditional_t< is_task_v<R>, R, TaskImpl<R> >
    {
        auto thenCb { std::forward<ThenCallback>(thenCallback) };
        auto errCb { std::forward<ErrorCallback>(errorCallback) };
        auto && taskImpl { static_cast< TaskImpl<T> const & >(task) };

#undef CATCH_TASK_EXP
#define CATCH_TASK_EXP catch (std::exception const & e) { co_return handleException<R>(errCb, e); }

        if constexpr (std::is_void_v<typename TaskImpl<T>::value_type>) {
            try { co_await taskImpl; }
            CATCH_TASK_EXP

            if constexpr (is_task_v<R>) { co_return co_await invokeCb(thenCb); }
            else { co_return invokeCb(thenCb); }
        } else {
            std::optional<T> value {};

            try { value.emplace(std::move(co_await taskImpl)); }
            CATCH_TASK_EXP

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
            CATCH_TASK_EXP

            if constexpr (is_task_v<R>) { co_return co_await invokeCb(thenCb); }
            else { co_return invokeCb(thenCb); }
        } else {
            std::optional<T> value {};

            try { value.emplace(std::move(co_await taskImpl)); }
            CATCH_TASK_EXP

            if constexpr (is_task_v<R>) { co_return co_await invokeCb(thenCb , std::move(*value)); }
            else { co_return invokeCb(thenCb , std::move(*value)); }
        }
    }

#undef CATCH_TASK_EXP
#undef XCoroTaskAbstractClass
#undef XCoroTaskAbstractClassTemplate

}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
