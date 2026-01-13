#ifndef XUTILS2_X_COROUTINE_TASK_HPP
#define XUTILS2_X_COROUTINE_TASK_HPP 1

#pragma once

#include <XGlobal/xqt_detection.hpp>
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

template<typename T = void>
class XCoroTask;

namespace detail {

    using coroutine_handle_vector = std::vector<std::coroutine_handle<>>;

    class TaskFinalSuspend {
        coroutine_handle_vector m_awaitingCoroutines_ {};
    public:
        explicit(false) constexpr TaskFinalSuspend(coroutine_handle_vector && awaitingCoroutines);

        static constexpr bool await_ready() noexcept;

        template<typename Promise>
        void await_suspend(std::coroutine_handle<Promise> finishedCoroutine) noexcept;

        static constexpr void await_resume() noexcept;
    };

    class TaskPromiseAbstract : public AwaitTransformMixin {
        friend class TaskFinalSuspend;
        coroutine_handle_vector m_awaitingCoroutines_ {};
        XAtomicInteger<uint32_t> m_ref_ {1};

    public:
        static constexpr std::suspend_never initial_suspend() noexcept;
        constexpr TaskFinalSuspend final_suspend() noexcept;

        constexpr void addAwaitingCoroutine(std::coroutine_handle<>);
        constexpr bool hasAwaitingCoroutine() const noexcept;

        void derefCoroutine();
        void refCoroutine() noexcept;
        void destroyCoroutine();

        constexpr virtual ~TaskPromiseAbstract() = default;

    protected:
        explicit(false) constexpr TaskPromiseAbstract() = default;
    };

    template<typename T>
    class TaskPromise: public TaskPromiseAbstract {
        std::variant<std::monostate, T, std::exception_ptr> m_value_ {};
    public:
        constexpr XCoroTask<T> get_return_object() noexcept;
        void unhandled_exception();

        constexpr void return_value(T && value) noexcept;
        constexpr void return_value(T const & value) noexcept;
        template<typename U> requires std::constructible_from<T, U>
        constexpr void return_value(U && value) noexcept;

        constexpr T & result() &;
        constexpr T && result() &&;

        constexpr ~TaskPromise() override = default;
    };

    template<>
    class TaskPromise<void> : public TaskPromiseAbstract {
        std::exception_ptr m_exception_ {};
    public:
        XCoroTask<> get_return_object() noexcept;
        void unhandled_exception();
        static constexpr void return_void() noexcept;
        void result() const;
        constexpr ~TaskPromise() override = default;
    };

    template<typename Promise>
    class TaskAwaiterAbstract {
    protected:
        using coroutine_handle = std::coroutine_handle<Promise>;
        coroutine_handle m_awaitedCoroutine_ {};

    public:
        [[nodiscard]] constexpr bool await_ready() const noexcept;
        constexpr void await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept;

    protected:
       explicit constexpr TaskAwaiterAbstract(coroutine_handle promise) noexcept;
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
        constexpr XCoroTaskAbstract(XCoroTaskAbstract &&) noexcept;

        constexpr XCoroTaskAbstract & operator=(XCoroTaskAbstract && ) noexcept;

        virtual ~XCoroTaskAbstract();

        [[nodiscard]] constexpr bool isReady() const;

        auto operator co_await() const noexcept;

        constexpr void swap(XCoroTaskAbstract &) noexcept;

        template<typename ThenCallback> requires (
            std::is_invocable_v<ThenCallback>
            || (!std::is_void_v<T> && std::is_invocable_v<ThenCallback, T>)
        )
        constexpr auto then(ThenCallback && callback) &;

        template<typename ThenCallback> requires (
            std::is_invocable_v<ThenCallback>
            || (!std::is_void_v<T> && std::is_invocable_v<ThenCallback, T>)
        )
        constexpr auto then(ThenCallback && callback) &&;

        template<typename ThenCallback, typename ErrorCallback>
        requires (
            ( std::is_invocable_v<ThenCallback> || (!std::is_void_v<T> && std::is_invocable_v<ThenCallback, T>) )
            && std::is_invocable_v<ErrorCallback, std::exception const &>
        )
        auto then(ThenCallback && callback, ErrorCallback &&errorCallback) &;

        template<typename ThenCallback, typename ErrorCallback>
        requires (
            ( std::is_invocable_v<ThenCallback> || (!std::is_void_v<T> && std::is_invocable_v<ThenCallback, T>) )
            && std::is_invocable_v<ErrorCallback, std::exception const &>
        )
        auto then(ThenCallback && callback, ErrorCallback && errorCallback) &&;

    private:
        template<typename ThenCallback, typename ... Args>
        static constexpr auto invokeCb(ThenCallback && callback, [[maybe_unused]] Args && ... args)
            noexcept(std::is_nothrow_invocable_v<ThenCallback,decltype(std::declval<Args>())...>);

        template<typename R, typename ErrorCallback , typename U = is_task_rt<R>>
        static constexpr U handleException(ErrorCallback && errCb, const std::exception &exception);

        template<typename ThenCallback, typename ...Arg>
        struct cb_invoke_result: std::conditional_t<
            std::is_invocable_v<ThenCallback>,
                std::invoke_result<ThenCallback>,
                std::invoke_result<ThenCallback, Arg...>
            > {};

        template<typename ThenCallback>
        struct cb_invoke_result<ThenCallback, void>: std::invoke_result<ThenCallback> {};

        template<typename ThenCallback, typename ...Arg>
        using cb_invoke_result_t = cb_invoke_result<ThenCallback, Arg...>::type;

        template<typename TaskT, typename ThenCallback, typename ErrorCallback, typename R = cb_invoke_result_t<ThenCallback, T>>
        static constexpr auto thenImpl(TaskT task, ThenCallback && thenCallback, ErrorCallback && errorCallback)
            -> std::conditional_t< is_task_v<R>, R, TaskImpl<R> >;

        template<typename TaskT, typename ThenCallback, typename ErrorCallback, typename R = cb_invoke_result_t<ThenCallback, T>>
        static constexpr auto thenImplRef(TaskT & task, ThenCallback && thenCallback, ErrorCallback && errorCallback)
            -> std::conditional_t<is_task_v<R>, R, TaskImpl<R>>;

        template<typename TaskT, typename ThenCallback, typename ErrorCallback, typename R = cb_invoke_result_t<ThenCallback, T>>
        static auto thenImplCore(TaskT && , ThenCallback && thenCallback, ErrorCallback && errorCallback)
            -> std::conditional_t<is_task_v<R>, R, TaskImpl<R>>;

    protected:
        constexpr XCoroTaskAbstract() = default;
        explicit(false) constexpr XCoroTaskAbstract(coroutine_handle) noexcept;
        X_DISABLE_COPY(XCoroTaskAbstract)
    };

    template <typename T>
    concept TaskConvertible = requires(T val, TaskPromiseAbstract promise)
    { { promise.await_transform(val) }; };

    template<typename T>
    struct awaitable_return_type
    { using type = std::decay_t< decltype(std::declval<T>().await_resume()) >; };

    template<has_member_operator_coawait T>
    struct awaitable_return_type<T>
    { using type = std::decay_t< typename awaitable_return_type< decltype(std::declval<T>().operator co_await()) >::type >; };

    template<has_nonmember_operator_coawait T>
    struct awaitable_return_type<T>
    { using type = std::decay_t< typename awaitable_return_type< decltype(operator co_await(std::declval<T>())) >::type >; };

    template<Awaitable Awaitable>
    using awaitable_return_type_t = awaitable_return_type<Awaitable>::type;

    template <typename Awaitable> requires TaskConvertible<Awaitable>
    using convertible_awaitable_return_type_t = awaitable_return_type_t< decltype(std::declval<TaskPromiseAbstract>().await_transform(std::declval<Awaitable>())) >;

}/* namespace detail */

template<typename T>
class XCoroTask final :
    public detail::XCoroTaskAbstract<T,XCoroTask, detail::TaskPromise<T>>
{
    using Base = detail::XCoroTaskAbstract<T, XCoroTask, detail::TaskPromise<T>>;
    using coroutine_handle = Base::coroutine_handle;

public:
    //! Promise type of the coroutine. This is required by the C++ standard.
    using promise_type = detail::TaskPromise<T>;
    //! The type of the coroutine return value.
    using value_type = T;

    [[nodiscard]] constexpr const auto * coroutineHandle() const noexcept
    { return std::addressof(this->m_coroutine_); }

    constexpr XCoroTask() noexcept = default;

    explicit(false) constexpr XCoroTask(std::coroutine_handle<detail::TaskPromise<T>> coroutine)
        : Base(coroutine) {}
};

template<typename T>
constexpr T waitFor(XCoroTask<T> &task);

// \overload
template<typename T>
constexpr T waitFor(XCoroTask<T> && task);

// \overload
template<Awaitable Awaitable>
constexpr auto waitFor(Awaitable && awaitable);

#ifdef HAS_QT
template <typename T, typename QObjectSubclass, typename Callback>
requires std::is_invocable_v<Callback>
    || std::is_invocable_v<Callback, T>
    || std::is_invocable_v<Callback, QObjectSubclass *>
    || std::is_invocable_v<Callback, QObjectSubclass *, T>
void connect(XCoroTask<T> && task, QObjectSubclass * context, Callback && func);

template <typename T, typename QObjectSubclass, typename Callback>
requires detail::TaskConvertible<T>
        && (std::is_invocable_v<Callback>
            || std::is_invocable_v<Callback, detail::convertible_awaitable_return_type_t<T>>
            || std::is_invocable_v<Callback, QObjectSubclass *>
            || std::is_invocable_v<Callback, QObjectSubclass *, detail::convertible_awaitable_return_type_t<T>>)
        && (!detail::is_task_v<T>)
void connect(T && future, QObjectSubclass *context, Callback && func);
#endif

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#define X_COROUTINE_
#include <XCoroutine/impl/taskawaiterabstract.hpp>
#include <XCoroutine/impl/taskfinalsuspend.hpp>
#include <XCoroutine/impl/taskpromiseabstract.hpp>
#include <XCoroutine/impl/xcorotaskabstract.hpp>
#include <XCoroutine/impl/taskpromise.hpp>
#include <XCoroutine/impl/waitfor.hpp>
#include <XCoroutine/impl/connect.hpp>
#undef X_COROUTINE_

#endif
