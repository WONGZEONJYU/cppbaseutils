#ifndef XUTILS2_TASK_PROMISE_HPP
#define XUTILS2_TASK_PROMISE_HPP 1

#pragma once

#include <cassert>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    template<typename T>
    constexpr auto TaskPromise<T>::get_return_object() noexcept -> XCoroTask<T>
    { return XCoroTask<T> { std::coroutine_handle<TaskPromise>::from_promise(*this) }; }

    template<typename T>
    constexpr void TaskPromise<T>::unhandled_exception()
    { m_value_ = std::current_exception(); }

    template<typename T>
    constexpr void TaskPromise<T>::return_value(T && value) noexcept
    { m_value_.template emplace<T>(std::forward<T>(value)); }

    template<typename T>
    constexpr void TaskPromise<T>::return_value(T const & value) noexcept
    { m_value_ = value; }

    template<typename T>
    template<typename U> requires std::constructible_from<T, U>
    constexpr void TaskPromise<T>::return_value(U && value) noexcept
    { m_value_ = T { std::forward<U>(value) }; }

#define HAS_EXCEPTION() do { \
    if (std::holds_alternative<std::exception_ptr>(m_value_)) { \
        assert(std::get<std::exception_ptr>(m_value_)); \
        std::rethrow_exception(std::get<std::exception_ptr>(m_value_)); \
    } }while (false)

    template<typename T>
    constexpr T & TaskPromise<T>::result() & {
        HAS_EXCEPTION();
        return std::get<T>(m_value_);
    }

    template<typename T>
    constexpr T && TaskPromise<T>::result() && {
        HAS_EXCEPTION();
        return std::move(std::get<T>(m_value_));
    }

#undef HAS_EXCEPTION

    constexpr auto TaskPromise<void>::get_return_object() noexcept -> XCoroTask<>
    { return XCoroTask { std::coroutine_handle<TaskPromise>::from_promise(*this) }; }

    constexpr void TaskPromise<void>::unhandled_exception()
    { m_exception_ = std::current_exception(); }

    constexpr void TaskPromise<void>::return_void() noexcept {}

    constexpr void TaskPromise<void>::result() const
    { if (m_exception_) { std::rethrow_exception(m_exception_); } }

}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
