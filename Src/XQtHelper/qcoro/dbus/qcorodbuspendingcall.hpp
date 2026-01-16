#ifndef XUTILS2_Q_CORO_DBUS_PENDING_CALL_HPP
#define XUTILS2_Q_CORO_DBUS_PENDING_CALL_HPP 1

#pragma once

#include <XCoroutine/xcoroutinetask.hpp>
#include <XQtHelper/qcoro/core/qcorosignal.hpp>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusMessage>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    class QCoroDBusPendingCall {
        friend struct awaiter_type<QDBusPendingCall>;
        QDBusPendingCall const * m_call_ {};

        class WaitForFinishedOperation {
            QDBusPendingCall const * m_call_ {};
        public:
            explicit(false) constexpr WaitForFinishedOperation(QDBusPendingCall const & call)
                : m_call_ { std::addressof(call) } {}

            [[nodiscard]] bool await_ready() const noexcept
            { return m_call_->isFinished(); }

            void await_suspend(std::coroutine_handle<> const h) const noexcept {
                auto slot { [h]<typename Tp_>(Tp_ && watcher_){ h.resume(); watcher_->deleteLater(); } };
                auto const watcher { std::make_unique<QDBusPendingCallWatcher>(*m_call_).release() };
                QObject::connect(watcher, &QDBusPendingCallWatcher::finished,std::move(slot));
            }

            [[nodiscard]] QDBusMessage await_resume() const
            { Q_ASSERT(m_call_->isFinished()); return m_call_->reply(); }
        };

    public:
        explicit(false) QCoroDBusPendingCall(QDBusPendingCall const & call)
            : m_call_ { std::addressof(call) } {}

        [[nodiscard]] XCoroTask<QDBusMessage> waitForFinished() const {
            QDBusPendingCallWatcher watcher {*m_call_};
            co_await qCoro(std::addressof(watcher), &QDBusPendingCallWatcher::finished);
            co_return watcher.reply();
        }
    };

    template<> struct awaiter_type<QDBusPendingCall> { using type = QCoroDBusPendingCall::WaitForFinishedOperation; };

}

inline auto qCoro(QDBusPendingCall const & call) noexcept
{ return detail::QCoroDBusPendingCall{call}; }

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
