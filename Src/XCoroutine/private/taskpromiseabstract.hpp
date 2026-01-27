#ifndef XUTILS2_TASK_PROMISE_ABSTRACT_HPP
#define XUTILS2_TASK_PROMISE_ABSTRACT_HPP 1

#ifndef X_COROUTINE_
#error Do not taskpromiseabstract.hpp directly
#endif

#pragma once

#include <XGlobal/xclasshelpermacros.hpp>
#include <XAtomic/xatomic.hpp>
#include <XCoroutine/private/mixns.hpp>
#include <coroutine>
#include <vector>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    using coroutine_handle_vector = std::vector<std::coroutine_handle<>>;

    class TaskFinalSuspend final {
        coroutine_handle_vector m_awaitingCoroutines_ {};
    public:
        X_IMPLICIT constexpr TaskFinalSuspend(coroutine_handle_vector && awaitingCoroutines)
            : m_awaitingCoroutines_ { std::move(awaitingCoroutines) }
        {   }

        static constexpr bool await_ready() noexcept { return {}; }

        template<typename Promise>
        void await_suspend(std::coroutine_handle<Promise> const h) noexcept {
            auto && promise{ h.promise() };
            for (auto && awaiter : m_awaitingCoroutines_)
            { awaiter.resume(); }
            m_awaitingCoroutines_.clear();
            promise.derefCoroutine();
        }

        static constexpr void await_resume() noexcept {}
    };

    class TaskPromiseAbstract : public AwaitTransformMixin {
        friend class TaskFinalSuspend;
        coroutine_handle_vector m_awaitingCoroutines_ {};
        XAtomicInteger<uint32_t> m_ref_ {1};

    public:
        static constexpr auto initial_suspend() noexcept
        { return std::suspend_never {}; }

        constexpr auto final_suspend() noexcept
        { return TaskFinalSuspend {std::move(m_awaitingCoroutines_) }; }

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
