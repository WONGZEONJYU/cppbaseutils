#ifndef XUTILS2_TASK_PROMISE_ABSTRACT_HPP
#define XUTILS2_TASK_PROMISE_ABSTRACT_HPP 1

#pragma once

#include <XCoroutine/xcoroutinetask.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    constexpr std::suspend_never TaskPromiseAbstract::initial_suspend() noexcept
    { return {}; }

    constexpr auto TaskPromiseAbstract::final_suspend() const noexcept
    { return TaskFinalSuspend{m_awaitingCoroutines_}; }

    constexpr void TaskPromiseAbstract::addAwaitingCoroutine(std::coroutine_handle<> const awaitingCoroutine)
    { m_awaitingCoroutines_.push_back(awaitingCoroutine); }

    constexpr bool TaskPromiseAbstract::hasAwaitingCoroutine() const noexcept
    { return !m_awaitingCoroutines_.empty(); }

    constexpr void TaskPromiseAbstract::derefCoroutine()
    { if (!m_ref_.deref()) { destroyCoroutine(); } }

    constexpr void TaskPromiseAbstract::refCoroutine() noexcept
    { m_ref_.ref(); }

    constexpr void TaskPromiseAbstract::destroyCoroutine() {
        m_ref_.storeRelaxed({});
        auto const handle { std::coroutine_handle<TaskPromiseAbstract>::from_promise(*this) };
        handle.destroy();
    }
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
