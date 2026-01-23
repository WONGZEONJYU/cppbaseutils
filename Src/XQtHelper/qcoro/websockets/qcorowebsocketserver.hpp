#ifndef XUTILS2_Q_CORO_WEBSOCKET_SERVER_HPP
#define XUTILS2_Q_CORO_WEBSOCKET_SERVER_HPP 1

#pragma once

#include <XCoroutine/xcoroutinetask.hpp>
#include <XQtHelper/qcoro/core/qcorosignal.hpp>
#include <chrono>
#include <QWebSocketServer>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    struct QCoroWebSocketServerSignalListener : QObject {
        Q_OBJECT
    public:
        explicit(false) QCoroWebSocketServerSignalListener(QWebSocketServer * const server) {
            connect(server, &QWebSocketServer::closed, this, [this]{ Q_EMIT ready({});});
            connect(server, &QWebSocketServer::newConnection, [this, server]{ Q_EMIT ready(server->nextPendingConnection()); });
        }

        Q_SIGNAL void ready(QWebSocket *);
    };

    class QCoroWebSocketServer {
        QWebSocketServer *m_server_{};
    public:
        explicit(false) QCoroWebSocketServer(QWebSocketServer * const server)
            : m_server_(server)
        {   }

        using milliseconds = std::chrono::milliseconds;
        XCoroTask<QWebSocket *> nextPendingConnection(milliseconds const timeout = milliseconds{-1}) const {
            auto const server { m_server_ };
            if (!server->isListening()) { co_return nullptr; }
            if (server->hasPendingConnections()) { co_return server->nextPendingConnection(); }
            QCoroWebSocketServerSignalListener listener { server };
            auto const result { co_await qCoro(std::addressof(listener), &QCoroWebSocketServerSignalListener::ready, timeout) };
            co_return result.value_or(nullptr);
        }
    };
}

inline auto qCoro(QWebSocketServer * const server) noexcept
{return detail::QCoroWebSocketServer{server}; }

inline auto qCoro(QWebSocketServer & server) noexcept
{ return detail::QCoroWebSocketServer {std::addressof(server)}; }

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
