#ifndef XUTILS2_Q_CORO_CHRONO_TIMER_HPP
#define XUTILS2_Q_CORO_CHRONO_TIMER_HPP 1

#pragma once

#include <QtVersionChecks>
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)

#include <XCoroutine/xcoroutinetask.hpp>
#include <XQtHelper/qcoro/core/qcorosignal.hpp>
#include <QMetaObject>
#include <QPointer>
#include <QChronoTimer>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    class QCoroChronoTimer {
        QPointer<QChronoTimer> m_timer_{};

        class WaitForTimeoutOperation {
            QMetaObject::Connection m_conn_{};
            QPointer<QChronoTimer> m_timer_{};
        public:
            Q_IMPLICIT WaitForTimeoutOperation(QChronoTimer * const timer) noexcept
                : m_timer_ { timer }
            {    }

            Q_IMPLICIT WaitForTimeoutOperation(QChronoTimer & timer) noexcept
                : WaitForTimeoutOperation { std::addressof(timer) }
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
        Q_IMPLICIT QCoroChronoTimer(QChronoTimer * const timer) noexcept
            : m_timer_ {timer}
        {   }

        Q_IMPLICIT QCoroChronoTimer(QChronoTimer & timer) noexcept
            : QCoroChronoTimer { std::addressof(timer) }
        {   }

        [[nodiscard]] XCoroTaskVoid waitForTimeout() const
        { if (m_timer_->isActive()) { co_await qCoro(m_timer_.data(), &QChronoTimer::timeout); } }

        template<typename > friend struct awaiter_type;
    };

    template<>
    struct awaiter_type<QChronoTimer *> { using type = QCoroChronoTimer::WaitForTimeoutOperation; };

    template<>
    struct awaiter_type<QChronoTimer> { using type = QCoroChronoTimer::WaitForTimeoutOperation; };

}

template<typename Rep, typename Period>
XCoroTaskVoid chronoSleepFor(std::chrono::duration<Rep, Period> const & timeout) {
    QChronoTimer timer {};
    timer.setSingleShot(true);
    timer.setInterval(std::chrono::duration_cast<std::chrono::nanoseconds>(timeout));
    timer.start();
    co_await timer;
}

template<typename Clock, typename Duration>
XCoroTaskVoid chronoSleepUntil(std::chrono::time_point<Clock, Duration> const & when)
{ return chronosleepFor(when.time_since_epoch() - std::chrono::steady_clock::now().time_since_epoch()); }

inline auto qCoro(QChronoTimer * const timer) noexcept
{ return detail::QCoroChronoTimer{ timer }; }

inline auto qCoro(QChronoTimer & timer) noexcept
{ return detail::QCoroChronoTimer{ timer }; }

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif

#endif
