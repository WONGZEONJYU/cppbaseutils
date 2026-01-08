#ifndef XUTILS2_X_COROUTINE_TASK_HPP
#define XUTILS2_X_COROUTINE_TASK_HPP 1

#pragma once

#include <XGlobal/xclasshelpermacros.hpp>
#include <XHelper/xversion.hpp>
#include <XAtomic/xatomic.hpp>
#include <XHelper/xutility.hpp>

#include <XCoroutine/private/coroutine_p.hpp>

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
        explicit constexpr TaskFinalSuspend(coroutine_handle_vector awaitingCoroutines);

        static constexpr bool await_ready() noexcept;

        template<typename Promise>
        constexpr void await_suspend(std::coroutine_handle<Promise> finishedCoroutine) noexcept;

        static constexpr void await_resume() noexcept;
    };

    class TaskPromiseAbstract {

        friend class TaskFinalSuspend;
        coroutine_handle_vector m_awaitingCoroutines_ {};
        XAtomicInteger<uint32_t> m_ref_ {1};

    public:
        static constexpr std::suspend_never initial_suspend() noexcept;
        constexpr auto final_suspend() const noexcept;

        constexpr void addAwaitingCoroutine(std::coroutine_handle<> awaitingCoroutine);
        constexpr bool hasAwaitingCoroutine() const noexcept;

        constexpr void derefCoroutine();
        constexpr void refCoroutine() noexcept;
        constexpr void destroyCoroutine();

        constexpr virtual ~TaskPromiseAbstract() = default;

    protected:
        explicit constexpr TaskPromiseAbstract() = default;
    };

    template<typename T>
    class TaskPromise: public TaskPromiseAbstract {

        std::variant<std::monostate, T, std::exception_ptr> m_value_ {};

    public:
        constexpr auto get_return_object() noexcept -> XCoroTask<T>;
        constexpr void unhandled_exception();
        constexpr void return_value(T && value) noexcept;
        constexpr void return_value(T const & value) noexcept;

        template<typename U> requires std::constructible_from<T, U>
        constexpr void return_value(U &&value) noexcept;

        constexpr T & result() &;
        constexpr T && result() &&;

        using TaskPromiseAbstract::TaskPromiseAbstract;
        constexpr ~TaskPromise() override = default;
    };

    template<>
    class TaskPromise<void> : public TaskPromiseAbstract {
        std::exception_ptr m_exception_ {};

    public:
        constexpr auto get_return_object() noexcept;
        constexpr void unhandled_exception();
        static constexpr void return_void() noexcept;
        constexpr void result() const;

        using TaskPromiseAbstract::TaskPromiseAbstract;
        constexpr ~TaskPromise() override = default;
    };

    template<typename Promise>
    class TaskAwaiterAbstract {
    protected:
        std::coroutine_handle<Promise> m_awaitedCoroutine_ {};

    public:
        [[nodiscard]] constexpr bool await_ready() const noexcept;
        constexpr void await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept;

    protected:
        explicit constexpr TaskAwaiterAbstract(std::coroutine_handle<Promise> promise) noexcept;
    };

    template<typename T>
    struct is_task : std::false_type { using return_type = T; };

    template<typename T>
    struct is_task<XCoroTask<T>> : std::true_type
    { using return_type = typename XCoroTask<T>::value_type; };

    template<typename T>
    inline constexpr auto is_task_v { is_task<T>::value };

    template<typename T>
    using is_task_rt = typename is_task<T>::return_type;

    template<typename T, template<typename> class TaskImpl, typename PromiseType>
    class XCoroTaskAbstract {
    protected:
        std::coroutine_handle<PromiseType> m_coroutine_ {};

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
        constexpr auto then(ThenCallback &&callback) &;

        template<typename ThenCallback> requires (
            std::is_invocable_v<ThenCallback>
            || (!std::is_void_v<T> && std::is_invocable_v<ThenCallback, T>)
        )
        constexpr auto then(ThenCallback &&callback) &&;

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
        static auto handleException(ErrorCallback &errCb, const std::exception &exception) -> U;

        template<typename F, typename Arg>
        using cb_invoke_result_t =
            std::conditional_t<
                std::is_void_v<Arg>,
                std::invoke_result_t<F>,
                std::invoke_result_t<F, Arg>
            >;

        template<typename TaskT, typename ThenCallback, typename ErrorCallback, typename R = cb_invoke_result_t<ThenCallback, T>>
        static auto thenImpl(TaskT task, ThenCallback &&thenCallback, ErrorCallback &&errorCallback)
            -> std::conditional_t< is_task_v<R>, R, TaskImpl<R> >;

        template<typename TaskT, typename ThenCallback, typename ErrorCallback, typename R = cb_invoke_result_t<ThenCallback, T>>
        static auto thenImplRef(TaskT &task, ThenCallback &&thenCallback, ErrorCallback &&errorCallback)
            -> std::conditional_t<is_task_v<R>, R, TaskImpl<R>>;

    protected:
        explicit constexpr XCoroTaskAbstract() = default;
        explicit constexpr XCoroTaskAbstract(std::coroutine_handle<PromiseType> coroutine) noexcept;
        X_DISABLE_COPY(XCoroTaskAbstract)
    };


}//namespace detail

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
