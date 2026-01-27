#ifndef XUTILS2_X_COROUTINE_TASK_HPP
#define XUTILS2_X_COROUTINE_TASK_HPP 1

#pragma once

#include <XGlobal/xversion.hpp>

#define X_COROUTINE_
#include <XCoroutine/private/taskpromise.hpp>
#include <XCoroutine/private/xcorotaskabstract.hpp>
#undef X_COROUTINE_

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template<typename T = void>
class XCoroTask final :
    public detail::XCoroTaskAbstract<T,XCoroTask, detail::TaskPromise<T>>
{
    using Base = detail::XCoroTaskAbstract<T, XCoroTask, detail::TaskPromise<T>>;
    using coroutine_handle = Base::coroutine_handle;

public:
    using value_type = T;
    using promise_type = detail::TaskPromise<value_type>;

    constexpr XCoroTask() noexcept = default;

    X_IMPLICIT constexpr XCoroTask(coroutine_handle const coroutine) noexcept
        : Base { coroutine }
    {    }

    X_IMPLICIT constexpr XCoroTask(promise_type & promise) noexcept
        : XCoroTask { coroutine_handle::from_promise(promise) }
    {    }

    X_IMPLICIT constexpr XCoroTask(promise_type * const promise) noexcept
        : XCoroTask { *promise }
    {    }
};

using XCoroTaskVoid = XCoroTask<>;

namespace detail {

    inline XCoroTaskVoid TaskPromiseVoid::get_return_object() noexcept
    { return { this }; }

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

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
