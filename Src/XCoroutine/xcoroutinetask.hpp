#ifndef XUTILS2_X_COROUTINE_TASK_HPP
#define XUTILS2_X_COROUTINE_TASK_HPP 1

#pragma once

#include <XGlobal/xclasshelpermacros.hpp>
#include <XGlobal/xversion.hpp>
#include <XAtomic/xatomic.hpp>

#define X_COROUTINE_
#include <XCoroutine/private/coroutine_p.hpp>
#include <XCoroutine/private/mixns_p.hpp>
#undef X_COROUTINE_

#include <vector>
#include <exception>
#include <variant>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template<typename T = void> class XCoroTask;

namespace detail {

    using coroutine_handle_vector = std::vector<std::coroutine_handle<>>;

    class TaskFinalSuspend {
        coroutine_handle_vector m_awaitingCoroutines_ {};
    public:
        explicit(false) constexpr TaskFinalSuspend(coroutine_handle_vector && awaitingCoroutines)
            : m_awaitingCoroutines_ { std::move(awaitingCoroutines) }
        {   }

        static constexpr bool await_ready() noexcept { return {}; };

        template<typename Promise>
        void await_suspend(std::coroutine_handle<Promise>) noexcept;

        static constexpr void await_resume() noexcept {};
    };

    class TaskPromiseAbstract : public AwaitTransformMixin {
        friend class TaskFinalSuspend;
        coroutine_handle_vector m_awaitingCoroutines_ {};
        XAtomicInteger<uint32_t> m_ref_ {1};

    public:
        static constexpr auto initial_suspend() noexcept
        { return std::suspend_never {}; }

        constexpr TaskFinalSuspend final_suspend() noexcept
        { return {std::move(m_awaitingCoroutines_) }; }

        constexpr void addAwaitingCoroutine(std::coroutine_handle<> const awaitingCoroutine)
        { m_awaitingCoroutines_.push_back(awaitingCoroutine); }

        constexpr bool hasAwaitingCoroutine() const noexcept
        { return !m_awaitingCoroutines_.empty(); }

        void derefCoroutine()
        { if (!m_ref_.deref()) { destroyCoroutine(); } }

        void refCoroutine() noexcept
        { m_ref_.ref(); }

        void destroyCoroutine();

        constexpr virtual ~TaskPromiseAbstract() = default;

    protected:
        constexpr TaskPromiseAbstract() noexcept = default;
    };

    template<typename T>
    class TaskPromise: public TaskPromiseAbstract {
        std::variant<std::monostate, T, std::exception_ptr> m_value_ {};
    public:
        constexpr XCoroTask<T> get_return_object() noexcept
        { return { std::coroutine_handle<TaskPromise>::from_promise(*this) }; }

        void unhandled_exception()
        { m_value_ = std::current_exception(); }

        constexpr void return_value(T && value) noexcept
        { m_value_.template emplace<T>(std::forward<T>(value)); }

        constexpr void return_value(T const & value) noexcept
        { m_value_ = value; }

        template<typename U> requires std::constructible_from<T, U>
        constexpr void return_value(U && value) noexcept
        { m_value_ = T(std::forward<U>(value)); }

        constexpr T & result() &;
        constexpr T && result() &&;

        constexpr ~TaskPromise() override = default;
    };

    template<>
    class TaskPromise<void> : public TaskPromiseAbstract {
        std::exception_ptr m_exception_ {};
    public:
        XCoroTask<> get_return_object() noexcept;

        void unhandled_exception()
        { m_exception_ = std::current_exception(); }

        static constexpr void return_void() noexcept {}

        void result() const
        { if (m_exception_) { std::rethrow_exception(m_exception_); } }

        constexpr ~TaskPromise() override = default;
    };

    template<typename Promise>
    class TaskAwaiterAbstract {
    protected:
        using coroutine_handle = std::coroutine_handle<Promise>;
        coroutine_handle m_awaitedCoroutine_ {};

    public:
        [[nodiscard]] constexpr bool await_ready() const noexcept
        { return m_awaitedCoroutine_ && m_awaitedCoroutine_.done(); }

        constexpr void await_suspend(std::coroutine_handle<>) noexcept;

    protected:
       explicit(false) constexpr TaskAwaiterAbstract(coroutine_handle const h) noexcept
        : m_awaitedCoroutine_ { h }
        {   }
    };

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
    protected:
        using coroutine_handle = std::coroutine_handle<PromiseType>;
        coroutine_handle m_coroutine_ {};

    public:
        constexpr XCoroTaskAbstract(XCoroTaskAbstract && o) noexcept
            : m_coroutine_ { std::move(o.m_coroutine_) }
        { o.m_coroutine_ = {}; }

        constexpr XCoroTaskAbstract & operator=(XCoroTaskAbstract && ) noexcept;

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
        constexpr auto then(ThenCallback && ) &;

        template<typename ThenCallback> requires (
            std::is_invocable_v<ThenCallback>
            || (!std::is_void_v<T> && std::is_invocable_v<ThenCallback, T>)
        )
        constexpr auto then(ThenCallback && ) &&;

        template<typename ThenCallback, typename ErrorCallback>
        requires (
            ( std::is_invocable_v<ThenCallback> || (!std::is_void_v<T> && std::is_invocable_v<ThenCallback, T>) )
            && std::is_invocable_v<ErrorCallback, std::exception const &>
        )
        auto then(ThenCallback && , ErrorCallback &&) &;

        template<typename ThenCallback, typename ErrorCallback>
        requires (
            ( std::is_invocable_v<ThenCallback> || (!std::is_void_v<T> && std::is_invocable_v<ThenCallback, T>) )
            && std::is_invocable_v<ErrorCallback, std::exception const &>
        )
        auto then(ThenCallback && , ErrorCallback && ) &&;

    private:
        template<typename ThenCallback, typename ... Args>
        static constexpr auto invokeCb(ThenCallback && , [[maybe_unused]] Args && ... )
            noexcept(std::is_nothrow_invocable_v<ThenCallback,decltype(std::declval<Args>())...>);

        template<typename R, typename ErrorCallback , typename U = is_task_rt<R>>
        static constexpr U handleException(ErrorCallback && , std::exception const &);

        template<typename ThenCallback, typename ...Arg>
        struct cb_invoke_result : std::conditional_t<
            std::is_invocable_v<ThenCallback>,
                std::invoke_result<ThenCallback>,
                std::invoke_result<ThenCallback, Arg...>
            > {};

        template<typename ThenCallback>
        struct cb_invoke_result<ThenCallback, void> : std::invoke_result<ThenCallback> {};

        template<typename ThenCallback, typename ...Arg>
        using cb_invoke_result_t = cb_invoke_result<ThenCallback, Arg...>::type;

        template<typename TaskT, typename ThenCallback, typename ErrorCallback, typename R = cb_invoke_result_t<ThenCallback, T>>
        static constexpr auto thenImpl(TaskT , ThenCallback && , ErrorCallback && )
            -> std::conditional_t< is_task_v<R>, R, TaskImpl<R> >;

        template<typename TaskT, typename ThenCallback, typename ErrorCallback, typename R = cb_invoke_result_t<ThenCallback, T>>
        static constexpr auto thenImplRef(TaskT & , ThenCallback && , ErrorCallback && )
            -> std::conditional_t<is_task_v<R>, R, TaskImpl<R>>;

        template<typename TaskT, typename ThenCallback, typename ErrorCallback, typename R = cb_invoke_result_t<ThenCallback, T>>
        static auto thenImplCore(TaskT && , ThenCallback && , ErrorCallback && )
            -> std::conditional_t<is_task_v<R>, R, TaskImpl<R>>;

    protected:
        constexpr XCoroTaskAbstract() noexcept = default;

        explicit(false) constexpr XCoroTaskAbstract(coroutine_handle const h) noexcept
            : m_coroutine_ { h }
        { m_coroutine_.promise().refCoroutine(); }

        X_DISABLE_COPY(XCoroTaskAbstract)
    };

    template <typename T>
    concept TaskConvertible = requires(T val, TaskPromiseAbstract promise)
    { { promise.await_transform(val) }; };

    template<typename T>
    struct awaitable_return_type
    { using type = std::decay_t< decltype(std::declval<T>().await_resume()) >; };

    template<Awaitable Awaitable>
    using awaitable_return_type_t = awaitable_return_type<Awaitable>::type;

    template<has_member_operator_coawait T>
    struct awaitable_return_type<T>
    { using type = std::decay_t< awaitable_return_type_t< decltype(std::declval<T>().operator co_await()) > >; };

    template<has_nonmember_operator_coawait T>
    struct awaitable_return_type<T>
    { using type = std::decay_t< awaitable_return_type_t< decltype(operator co_await(std::declval<T>())) > >; };

    template <typename Awaitable> requires TaskConvertible<Awaitable>
    using convertible_awaitable_return_type_t = awaitable_return_type_t< decltype(std::declval<TaskPromiseAbstract>().await_transform(std::declval<Awaitable>())) >;

}

template<typename T>
class XCoroTask final :
    public detail::XCoroTaskAbstract<T,XCoroTask, detail::TaskPromise<T>>
{
    using Base = detail::XCoroTaskAbstract<T, XCoroTask, detail::TaskPromise<T>>;
    using coroutine_handle = Base::coroutine_handle;

public:
    using value_type = T;
    using promise_type = detail::TaskPromise<value_type>;

    constexpr XCoroTask() noexcept = default;
    explicit(false) constexpr XCoroTask(coroutine_handle const coroutine)
        : Base { coroutine } {    }
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#define X_COROUTINE_
#include <XCoroutine/impl/taskawaiterabstract.hpp>
#include <XCoroutine/impl/taskfinalsuspend.hpp>
#include <XCoroutine/impl/taskpromiseabstract.hpp>
#include <XCoroutine/impl/xcorotaskabstract.hpp>
#include <XCoroutine/impl/taskpromise.hpp>
#undef X_COROUTINE_

#endif
