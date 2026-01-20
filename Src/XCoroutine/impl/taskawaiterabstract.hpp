#ifndef XUTILS2_TASK_AWAITER_ABSTRACT_HPP
#define XUTILS2_TASK_AWAITER_ABSTRACT_HPP 1

#ifndef X_COROUTINE_
#error Do not taskawaiterabstract.hpp directly
#endif

#pragma once

#include <iostream>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template <typename Promise>
constexpr void detail::TaskAwaiterAbstract<Promise>::await_suspend(std::coroutine_handle<> const h) noexcept {
    if (!m_awaitedCoroutine_) {
        std::cerr << "XUtils::XCoroTask: Awaiting a default-constructed or a moved-from XUtils::XCoroTask<> - this will hang forever!\n";
        return;
    }
    m_awaitedCoroutine_.promise().addAwaitingCoroutine(h);
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
