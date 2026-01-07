#ifndef XUTILS2_TASK_FINAL_SUSPEND_HPP
#define XUTILS2_TASK_FINAL_SUSPEND_HPP 1

#pragma once

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {
    constexpr TaskFinalSuspend::TaskFinalSuspend(coroutine_handle_vector awaitingCoroutines)
        : m_awaitingCoroutines_ {std::move(awaitingCoroutines ) }
    {}

    constexpr bool TaskFinalSuspend::await_ready() noexcept
    { return {}; }

    template<typename Promise>
    constexpr void TaskFinalSuspend::await_suspend(std::coroutine_handle<Promise> const finishedCoroutine) noexcept{
        auto && promise{ finishedCoroutine.promise() };
        for (auto && awaiter : m_awaitingCoroutines_)
        { awaiter.resume(); }
        m_awaitingCoroutines_.clear();
        promise.derefCoroutine();
    }

    constexpr void TaskFinalSuspend::await_resume() noexcept {}

}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
