#ifndef XUTILS2_TASK_FINAL_SUSPEND_HPP
#define XUTILS2_TASK_FINAL_SUSPEND_HPP 1

#ifndef X_COROUTINE_
#error Do not taskfinalsuspend.hpp directly
#endif

#pragma once

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template<typename Promise>
void detail::TaskFinalSuspend::await_suspend(std::coroutine_handle<Promise> const finishedCoroutine) noexcept {
    auto && promise{ finishedCoroutine.promise() };
    for (auto && awaiter : m_awaitingCoroutines_)
    { awaiter.resume(); }
    m_awaitingCoroutines_.clear();
    promise.derefCoroutine();
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
