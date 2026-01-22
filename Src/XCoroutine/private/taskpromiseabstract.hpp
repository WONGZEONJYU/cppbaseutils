#ifndef XUTILS2_TASK_PROMISE_ABSTRACT_HPP
#define XUTILS2_TASK_PROMISE_ABSTRACT_HPP 1

#ifndef X_COROUTINE_
#error Do not taskpromiseabstract.hpp directly
#endif

#pragma once

#include <XAtomic/xatomic.hpp>
#include <XCoroutine/private/mixns.hpp>
#include <XCoroutine/private/taskfinalsuspend.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    class TaskPromiseAbstract : public AwaitTransformMixin {
        friend class TaskFinalSuspend;
        coroutine_handle_vector m_awaitingCoroutines_ {};
        XAtomicInteger<uint32_t> m_ref_ {1};

    public:
        static constexpr auto initial_suspend() noexcept
        { return std::suspend_never {}; }

        constexpr auto final_suspend() noexcept
        { return TaskFinalSuspend{std::move(m_awaitingCoroutines_) }; }

        constexpr void addAwaitingCoroutine(std::coroutine_handle<> const awaitingCoroutine)
        { m_awaitingCoroutines_.push_back(awaitingCoroutine); }

        [[nodiscard]] constexpr bool hasAwaitingCoroutine() const noexcept
        { return !m_awaitingCoroutines_.empty(); }

        void derefCoroutine()
        { if (!m_ref_.deref()) { destroyCoroutine(); } }

        void refCoroutine() noexcept
        { m_ref_.ref(); }

        void destroyCoroutine() {
            m_ref_.storeRelaxed({});
            auto const handle { std::coroutine_handle<TaskPromiseAbstract>::from_promise(*this) };
            handle.destroy();
        }

        constexpr virtual ~TaskPromiseAbstract() = default;

    protected:
        constexpr TaskPromiseAbstract() noexcept = default;
    };

}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
