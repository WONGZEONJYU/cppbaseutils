#ifndef XUTILS2_TASK_AWAITER_ABSTRACT_HPP
#define XUTILS2_TASK_AWAITER_ABSTRACT_HPP 1

#ifndef X_COROUTINE_
#error Do not taskawaiterabstract.hpp directly
#endif

#pragma once

#include <iostream>
#include <coroutine>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    template<typename Promise>
    class TaskAwaiterAbstract {
    protected:
        using coroutine_handle = std::coroutine_handle<Promise>;
        coroutine_handle m_awaitedCoroutine_ {};

    public:
        [[nodiscard]] constexpr bool await_ready() const noexcept
        { return m_awaitedCoroutine_ && m_awaitedCoroutine_.done(); }

        constexpr void await_suspend(std::coroutine_handle<> const h) noexcept {
            if (!m_awaitedCoroutine_) {
                std::cerr << "XUtils::XCoroTask: Awaiting a default-constructed or a moved-from XUtils::XCoroTask<> - this will hang forever!\n";
                return;
            }
            m_awaitedCoroutine_.promise().addAwaitingCoroutine(h);
        }

    protected:
        explicit(false) constexpr TaskAwaiterAbstract(coroutine_handle const h) noexcept
         : m_awaitedCoroutine_ { h }
        {   }
    };

}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
