#ifndef XUTILS2_Q_CORO_ABSTRACT_SOCKET_HPP
#define XUTILS2_Q_CORO_ABSTRACT_SOCKET_HPP 1

#pragma once

#include <XQtHelper/qcoro/core/qcoroiodevice.hpp>
#include <chrono>
#include <QAbstractSocket>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    class AbstractSocketReadySignalHelper : public WaitSignalHelper {
        Q_OBJECT
        QMetaObject::Connection m_stateChanged_{};

    public:
        explicit(false) AbstractSocketReadySignalHelper(const QAbstractSocket * const socket, signalFunc<> const readySignal)
            : WaitSignalHelper { socket, readySignal }
        , m_stateChanged_ { connect_(socket, &QAbstractSocket::stateChanged, this,
                        [this](QAbstractSocket::SocketState const state) { handleStateChange(state,false); }) }
        {   }

        explicit(false) AbstractSocketReadySignalHelper(const QAbstractSocket * const socket, signalFunc<true> const readySignal)
            : WaitSignalHelper(socket, readySignal)
            , m_stateChanged_{ connect_(socket, &QAbstractSocket::stateChanged, this,
                            [this](QAbstractSocket::SocketState const state){
                                handleStateChange(state, static_cast<qint64>(0)); }) }
        {   }

    private:
        template<typename T>
        void handleStateChange(QAbstractSocket::SocketState const state, T const result) {
            if (state == QAbstractSocket::ClosingState || state == QAbstractSocket::UnconnectedState) {
                disconnect(m_stateChanged_);
                emitReady(result);
            }
        }
    };

    struct QCoroAbstractSocket final : QCoroIODevice {

        explicit(false) QCoroAbstractSocket(QAbstractSocket * const socket) : QCoroIODevice {socket} { }

        XCoroTask<bool> waitForConnected(int const timeout_msecs = 30'000)
        { return waitForConnected(milliseconds{ timeout_msecs }); }

        XCoroTask<bool> waitForConnected(milliseconds const timeout) {
            auto const socket { qobject_cast<QAbstractSocket *>(m_device_.data()) };
            if (socket->state() == QAbstractSocket::ConnectedState) { co_return true; }
            auto const result{ co_await qCoro(socket, &QAbstractSocket::connected, timeout) };
            co_return result.has_value();
        }

        XCoroTask<bool> waitForDisconnected(int const timeout_msecs = 30'000)
        { return waitForDisconnected(milliseconds{ timeout_msecs }); }

        XCoroTask<bool> waitForDisconnected(milliseconds const timeout) {
            auto const socket { qobject_cast<QAbstractSocket *>(m_device_.data()) };
            if (socket->state() == QAbstractSocket::UnconnectedState) { co_return false; }
            const auto result{ co_await qCoro(socket, &QAbstractSocket::disconnected, timeout) };
            co_return result.has_value();
        }

        XCoroTask<bool> connectToHost(QString const & hostName, quint16 const port,
                             QIODevice::OpenMode const openMode = QIODevice::ReadWrite,
                             QAbstractSocket::NetworkLayerProtocol const protocol = QAbstractSocket::AnyIPProtocol,
                             milliseconds const timeout = std::chrono::seconds{30})
        {
            qobject_cast<QAbstractSocket *>(m_device_.data())->connectToHost(hostName, port, openMode, protocol);
            return waitForConnected(timeout);
        }

        XCoroTask<bool> connectToHost(QHostAddress const & address, quint16 const port,
                             QIODevice::OpenMode const openMode = QIODevice::ReadWrite,
                             milliseconds const timeout = std::chrono::seconds{30})
        {
            qobject_cast<QAbstractSocket *>(m_device_.data())->connectToHost(address, port, openMode);
            return waitForConnected(timeout);
        }

    private:
        XCoroTask<std::optional<bool>> waitForReadyReadImpl(milliseconds const timeout) override {
            auto const socket { qobject_cast<QAbstractSocket *>(m_device_.data()) };
            if (socket->state() != QAbstractSocket::ConnectedState) { co_return false; }
            AbstractSocketReadySignalHelper helper { socket, &QIODevice::readyRead };
            co_return co_await qCoro(std::addressof(helper), qOverload<bool>(&WaitSignalHelper::ready), timeout);
        }

        XCoroTask<std::optional<qint64>> waitForBytesWrittenImpl(milliseconds const timeout) override {
            auto const socket { qobject_cast<QAbstractSocket *>(m_device_.data()) };
            if (socket->state() != QAbstractSocket::ConnectedState) { co_return std::nullopt; }
            AbstractSocketReadySignalHelper helper { socket, &QIODevice::bytesWritten };
            co_return co_await qCoro(std::addressof(helper), qOverload<qint64>(&WaitSignalHelper::ready), timeout);
        }
    };

}

inline auto qCoro(QAbstractSocket & s) noexcept
{ return detail::QCoroAbstractSocket{ std::addressof(s) }; }

inline auto qCoro(QAbstractSocket * const s) noexcept
{ return detail::QCoroAbstractSocket{ s }; }

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
