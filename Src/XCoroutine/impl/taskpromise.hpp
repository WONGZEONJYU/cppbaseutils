#ifndef XUTILS2_TASK_PROMISE_HPP
#define XUTILS2_TASK_PROMISE_HPP 1

#ifndef X_COROUTINE_
#error Do not taskpromise.hpp directly
#endif

#pragma once

#include <cassert>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

#define HAS_EXCEPTION() do { \
    if (std::holds_alternative<std::exception_ptr>(m_value_)) { \
        assert(std::get<std::exception_ptr>(m_value_)); \
        std::rethrow_exception(std::get<std::exception_ptr>(m_value_)); \
    } } while (false)

    template<typename T>
    constexpr T & TaskPromise<T>::result() &
    { HAS_EXCEPTION(); return std::get<T>(m_value_); }

    template<typename T>
    constexpr T && TaskPromise<T>::result() &&
    { HAS_EXCEPTION(); return std::move(std::get<T>(m_value_)); }

#undef HAS_EXCEPTION

    inline XCoroTask<> TaskPromise<void>::get_return_object() noexcept
    { return { std::coroutine_handle<TaskPromise>::from_promise(*this) }; }

}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
