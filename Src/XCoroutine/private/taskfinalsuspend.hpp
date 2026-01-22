#ifndef XUTILS2_TASK_FINAL_SUSPEND_HPP
#define XUTILS2_TASK_FINAL_SUSPEND_HPP 1

#include <XGlobal/xversion.hpp>
#include <coroutine>
#include <vector>

#ifndef X_COROUTINE_
#error Do not taskfinalsuspend.hpp directly
#endif

#pragma once

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    using coroutine_handle_vector = std::vector<std::coroutine_handle<>>;

    class TaskFinalSuspend final {
        coroutine_handle_vector m_awaitingCoroutines_ {};
    public:
        explicit(false) constexpr TaskFinalSuspend(coroutine_handle_vector && awaitingCoroutines)
            : m_awaitingCoroutines_ { std::move(awaitingCoroutines) }
        {   }

        static constexpr bool await_ready() noexcept { return {}; };

        template<typename Promise>
        void await_suspend(std::coroutine_handle<Promise> const h) noexcept {
            auto && promise{ h.promise() };
            for (auto && awaiter : m_awaitingCoroutines_)
            { awaiter.resume(); }
            m_awaitingCoroutines_.clear();
            promise.derefCoroutine();
        }

        static constexpr void await_resume() noexcept {};
    };

}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
