#ifndef XUTILS2_Q_CORO_TCPSERVER_HPP
#define XUTILS2_Q_CORO_TCPSERVER_HPP 1

#pragma once

#include <XCoroutine/xcoroutinetask.hpp>
#include <XQtHelper/qcoro/core/private/waitoperationabstract_p.hpp>
#include <XQtHelper/qcoro/core/qcorosignal.hpp>
#include <QTcpServer>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    class QCoroTcpServer {
        QPointer<QTcpServer> m_server_{};
        //! An Awaitable that suspends the coroutine until new connection is available
        struct WaitForNewConnectionOperation final
            : WaitOperationAbstract<QTcpServer>
        {
            explicit(false) WaitForNewConnectionOperation(QTcpServer * const server, int const timeout_msecs = 30'000)
                : WaitOperationAbstract { server, timeout_msecs }
            {   }

            bool await_ready() const noexcept
            { return !m_QObject_ || m_QObject_->hasPendingConnections(); }

            void await_suspend(std::coroutine_handle<> const h) {
                auto slot { [this,h]{ resume(h); } };
                m_conn_ = QObject::connect(m_QObject_.data(), &QTcpServer::newConnection,std::move(slot));
                startTimeoutTimer(h);
            }

            QTcpSocket * await_resume() const
            { return m_timedOut_ ? nullptr : m_QObject_->nextPendingConnection(); }
        };

    public:
        explicit(false) QCoroTcpServer(QTcpServer * const server) : m_server_ { server } { }

        using milliseconds = std::chrono::milliseconds;
        XCoroTask<QTcpSocket *> waitForNewConnection(int const timeout_msecs = 30'000) const
        { return waitForNewConnection(milliseconds{ timeout_msecs }); }

        XCoroTask<QTcpSocket *> waitForNewConnection(milliseconds const timeout) const {
            auto && server{ m_server_ };
            if (!server->isListening()) { co_return nullptr; }
            if (server->hasPendingConnections()) { co_return server->nextPendingConnection(); }
            if (auto const result{ co_await qCoro(server.data(), &QTcpServer::newConnection, timeout) };
                result.has_value())
            { co_return server->nextPendingConnection(); }
            co_return nullptr;
        }
    };

}

inline auto qCoro(QTcpServer & s) noexcept
{ return detail::QCoroTcpServer{std::addressof(s)}; }

inline auto qCoro(QTcpServer * const s) noexcept
{ return detail::QCoroTcpServer{s}; }

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
