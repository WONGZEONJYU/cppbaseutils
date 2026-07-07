#ifndef XUTILS2_TASK_PROMISE_ABSTRACT_HPP
#define XUTILS2_TASK_PROMISE_ABSTRACT_HPP 1

#ifndef X_COROUTINE_
#error Do not taskpromiseabstract.hpp directly
#endif

#pragma once

#include <XGlobal/xclasshelpermacros.hpp>
#include <XAtomic/xatomic.hpp>
#include <XCoroutine/private/mixns.hpp>
#include <XCoroutine/xcoromanager.hpp>
#include <coroutine>
#include <deque>
#include <vector>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

#if defined(X_PLATFORM_WINDOWS) && defined(X_COMPILER_MSVC)
    using coroutine_handle_container = std::vector<std::coroutine_handle<>>;
#else
    using coroutine_handle_container = std::deque<std::coroutine_handle<>>;
#endif

    class TaskFinalSuspend final {
        coroutine_handle_container m_awaitingCoroutines_ {};
    public:
        X_IMPLICIT TaskFinalSuspend(coroutine_handle_container && awaitingCoroutines)
            : m_awaitingCoroutines_ { std::move(awaitingCoroutines) }
        {   }

        static constexpr bool await_ready() noexcept { return {}; }

        template<typename Promise>
        void await_suspend(std::coroutine_handle<Promise> const h) noexcept {
            coroMgrRef().removeHandle(h);
            auto && promise{ h.promise() };
            for (auto && awaiter : m_awaitingCoroutines_)
            { awaiter.resume(); }
            m_awaitingCoroutines_ = coroutine_handle_container{};
            promise.derefCoroutine();
        }

        static constexpr void await_resume() noexcept {}
    };

    class TaskPromiseAbstract : public AwaitTransformMixin {
        friend class TaskFinalSuspend;
        coroutine_handle_container m_awaitingCoroutines_ {};
        XAtomicInteger<uint32_t> m_ref_ {1};

    public:
        static constexpr auto initial_suspend() noexcept
        { return std::suspend_never {}; }

        auto final_suspend() noexcept
        { return TaskFinalSuspend {std::move(m_awaitingCoroutines_) }; }

        void addAwaitingCoroutine(std::coroutine_handle<> const awaitingCoroutine)
        { m_awaitingCoroutines_.push_back(awaitingCoroutine); }

        [[nodiscard]] bool hasAwaitingCoroutine() const noexcept
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

        virtual ~TaskPromiseAbstract() = default;

    protected:
        constexpr TaskPromiseAbstract() noexcept = default;
    };

}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
