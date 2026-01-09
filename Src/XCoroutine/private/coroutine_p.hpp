#ifndef XUTILS2_COROUTINE_P_HPP
#define XUTILS2_COROUTINE_P_HPP

#pragma once

#include <utility>
#include <coroutine>
#include <XHelper/xversion.hpp>

#ifndef Q_MOC_RUN

#include <XCoroutine/private/concepts_p.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    template<typename T>
    concept has_await_methods = requires(T t) {
        { t.await_ready() } -> std::same_as<bool>;
        { t.await_suspend(std::declval<std::coroutine_handle<>>()) };
        { t.await_resume() };
    };

    template<typename T>
    concept has_member_operator_coawait = requires(T t) {
        // TODO: Check that result of co_await() satisfies Awaitable again
        { t.operator co_await() };
    };

    template<typename T>
    concept has_nonmember_operator_coawait = requires(T t) {
        // TODO: Check that result of the operator satisfied Awaitable again
#if defined(_MSC_VER) && !defined(__clang__)
        // FIXME: MSVC is unable to perform ADL lookup for operator co_await and just fails to compile
        { ::operator co_await(static_cast<T &&>(t)) };
#else
        { operator co_await(static_cast<T &&>(t)) };
#endif
    };

}

template<typename T>
concept Awaitable = detail::has_member_operator_coawait<T>
                    || detail::has_nonmember_operator_coawait<T>
                    || detail::has_await_methods<T>;

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
#endif
