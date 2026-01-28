#ifndef XUTILS2_Q_CORO_TIMER_HPP
#define XUTILS2_Q_CORO_TIMER_HPP 1

#pragma once

#include <XCoroutine/xcoroutinetask.hpp>
#include <XQtHelper/qcoro/core/qcorosignal.hpp>
#include <QMetaObject>
#include <QPointer>
#include <QTimer>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    class QCoroTimer {
        QPointer<QTimer> m_timer_{};

        class WaitForTimeoutOperation {
            QMetaObject::Connection m_conn_{};
            QPointer<QTimer> m_timer_{};
        public:
            Q_IMPLICIT WaitForTimeoutOperation(QTimer * const timer) noexcept
                : m_timer_ { timer }
            {    }

            Q_IMPLICIT WaitForTimeoutOperation(QTimer & timer) noexcept
                : m_timer_ { std::addressof(timer) }
            {   }

            [[nodiscard]] bool await_ready() const noexcept
            { return !m_timer_ || !m_timer_->isActive(); }

            void await_suspend(std::coroutine_handle<> const h) {
                if (!m_timer_ || !m_timer_->isActive()) { h.resume(); return; }
                m_conn_ = m_timer_->callOnTimeout([this, h]{ QObject::disconnect(m_conn_); h.resume(); });
            }

            static constexpr void await_resume() noexcept {}
        };

    public:
        Q_IMPLICIT QCoroTimer(QTimer * const timer) noexcept
            : m_timer_ {timer}
        {   }

        Q_IMPLICIT QCoroTimer(QTimer & timer) noexcept
            : QCoroTimer { std::addressof(timer) }
        {   }

        [[nodiscard]] XCoroTask<> waitForTimeout() const
        { if (m_timer_->isActive()) { co_await qCoro(m_timer_.data(), &QTimer::timeout); } }

        template<typename T> friend struct awaiter_type;
    };

    template<>
    struct awaiter_type<QTimer *> { using type = QCoroTimer::WaitForTimeoutOperation; };

    template<>
    struct awaiter_type<QTimer> { using type = QCoroTimer::WaitForTimeoutOperation; };

}

template<typename Rep, typename Period>
XCoroTask<> sleepFor(std::chrono::duration<Rep, Period> const & timeout) {
    QTimer timer {};
    timer.setSingleShot(true);
    using namespace std::chrono;
    timer.start(duration_cast<milliseconds>(timeout));
    co_await timer;
}

template<typename Clock, typename Duration>
XCoroTask<> sleepUntil(std::chrono::time_point<Clock, Duration> const & when)
{ return sleepFor(when.time_since_epoch() - std::chrono::steady_clock::now().time_since_epoch()); }

inline auto qCoro(QTimer * const timer) noexcept
{ return detail::QCoroTimer{ timer }; }

inline auto qCoro(QTimer & timer) noexcept
{ return detail::QCoroTimer{ timer }; }

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
