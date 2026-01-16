#ifndef XUTILS2_Q_CORO_DBUS_PENDING_REPLY_HPP
#define XUTILS2_Q_CORO_DBUS_PENDING_REPLY_HPP 1

#pragma once

#include <XCoroutine/xcoroutinetask.hpp>
#include <XQtHelper/qcoro/core/qcorosignal.hpp>
#include <QDBusPendingReply>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    template<typename ... Args>
    class QCoroDBusPendingReply {

        friend struct awaiter_type<QDBusPendingReply<Args ...>>;
        QDBusPendingReply<Args ...> m_reply_ {};

    #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        // QDBusPendingReply is a variadic template since Qt6, but in Qt5 the
        // maximum number of template arguments was 8, so we simulate the Qt5
        // behavior here.
        static_assert(sizeof...(Args) <= 8, "In Qt5 QDBusPendingReply has maximum 8 arguments.");
    #endif

        class WaitForFinishedOperation {
            QDBusPendingReply<Args ...> m_reply_{};
        public:
            explicit(false) constexpr WaitForFinishedOperation(QDBusPendingReply<Args ...> const & reply)
                : m_reply_ {reply} {}

            [[nodiscard]] bool await_ready() const noexcept
            { return m_reply_.isFinished(); }

            void await_suspend(std::coroutine_handle<> const awaitingCoroutine) const {
                auto slot { [awaitingCoroutine]<typename Tp_>(Tp_ && watcher_) {
                    awaitingCoroutine.resume();
                    std::forward<Tp_>(watcher_)->deleteLater();
                } };

                auto const watcher { std::make_unique<QDBusPendingCallWatcher>(m_reply_).release() };
                QObject::connect(watcher,&QDBusPendingCallWatcher::finished,std::move(slot));
            }

            QDBusPendingReply<Args ...> await_resume() const noexcept
            { Q_ASSERT(m_reply_.isFinished()); return m_reply_; }
        };

    public:
        explicit(false) constexpr QCoroDBusPendingReply(QDBusPendingReply<Args ...> const & reply)
            : m_reply_ {reply} {}

        XCoroTask<QDBusPendingReply<Args ...>> waitForFinished() const {
            if (m_reply_.isFinished()) { co_return m_reply_; }
            QDBusPendingCallWatcher watcher { m_reply_ };
            co_await qCoro(std::addressof(watcher), &QDBusPendingCallWatcher::finished);
            co_return watcher.reply();
        }
    };

    template<typename ... Args>
    struct awaiter_type<QDBusPendingReply<Args ...>> { using type = QCoroDBusPendingReply<Args ...>::WaitForFinishedOperation; };

}

template<typename ... Args>
auto qCoro(QDBusPendingReply<Args ...> const & reply) {
    return detail::QCoroDBusPendingReply<Args ...> {reply};
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
