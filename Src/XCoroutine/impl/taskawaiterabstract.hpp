#ifndef XUTILS2_TASK_AWAITER_ABSTRACT_HPP
#define XUTILS2_TASK_AWAITER_ABSTRACT_HPP 1

#pragma once

#include <iostream>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {
    template<typename Promise>
    constexpr bool TaskAwaiterAbstract<Promise>::await_ready() const noexcept
    { return m_awaitedCoroutine_ && m_awaitedCoroutine_.done(); }

    template <typename Promise>
    constexpr void TaskAwaiterAbstract<Promise>::await_suspend(std::coroutine_handle<> const awaitingCoroutine) noexcept {
        if (!m_awaitedCoroutine_) {
            std::cerr << "XUtils::XCoroTask: Awaiting a default-constructed or a moved-from XUtils::XCoroTask<> - this will hang forever!\n";
            return;
        }
        m_awaitedCoroutine_.promise().addAwaitingCoroutine(awaitingCoroutine);
    }

    template <typename Promise>
    constexpr TaskAwaiterAbstract<Promise>::TaskAwaiterAbstract(std::coroutine_handle<Promise> const promise) noexcept
        : m_awaitedCoroutine_ { promise }
    {}
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
