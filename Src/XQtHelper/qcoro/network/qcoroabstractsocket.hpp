#ifndef XUTILS2_Q_CORO_ABSTRACT_SOCKET_HPP
#define XUTILS2_Q_CORO_ABSTRACT_SOCKET_HPP 1

#pragma once

#include <XQtHelper/qcoro/core/qcoroiodevice.hpp>
#include <chrono>
#include <QAbstractSocket>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    struct AbstractSocketReadySignalHelper : WaitSignalHelper {
        Q_OBJECT
        QMetaObject::Connection m_stateChanged_{};
    public:
        Q_IMPLICIT AbstractSocketReadySignalHelper(QAbstractSocket const * const socket, signalFunc<> const readySignal)
            : WaitSignalHelper { socket, readySignal }
            , m_stateChanged_ { connect_(socket, &QAbstractSocket::stateChanged, this,
                [this]<typename Tp>(Tp && state){ handleStateChange(std::forward<Tp>(state),false); })
            }
        {   }

        Q_IMPLICIT AbstractSocketReadySignalHelper(QAbstractSocket const & socket, signalFunc<> const readySignal)
            : AbstractSocketReadySignalHelper { std::addressof(socket),readySignal }
        {   }

        Q_IMPLICIT AbstractSocketReadySignalHelper(QAbstractSocket const * const socket, signalFunc<qint64> const readySignal)
            : WaitSignalHelper { socket, readySignal }
            , m_stateChanged_{ connect_(socket, &QAbstractSocket::stateChanged, this,
                [this]<typename Tp>(Tp && state){ handleStateChange(std::forward<Tp>(state), static_cast<qint64>(0)); })
            }
        {   }

        Q_IMPLICIT AbstractSocketReadySignalHelper(QAbstractSocket const & socket, signalFunc<qint64> const readySignal)
            : AbstractSocketReadySignalHelper { std::addressof(socket),readySignal }
        {   }

    private:
        template<typename T>
        void handleStateChange(QAbstractSocket::SocketState const state, T && result) {
            if (state == QAbstractSocket::ClosingState || state == QAbstractSocket::UnconnectedState) {
                disconnect(m_stateChanged_);
                emitReady(std::forward<T>(result));
            }
        }
    };

    struct QCoroAbstractSocket final : QCoroIODevice {

        Q_IMPLICIT QCoroAbstractSocket(QAbstractSocket * const socket) noexcept
            : QCoroIODevice { socket }
        {   }

        Q_IMPLICIT QCoroAbstractSocket(QAbstractSocket & socket) noexcept
            : QCoroIODevice { socket }
        {   }

        CoroTaskBool waitForConnected(int const timeout_msecs = 30'000) const
        { return waitForConnected(milliseconds{ timeout_msecs }); }

        CoroTaskBool waitForConnected(milliseconds const timeout) const {
            auto const socket { qobject_cast<QAbstractSocket *>(m_device_.data()) };
            if (socket->state() == QAbstractSocket::ConnectedState) { co_return true; }
            auto const result{ co_await qCoro(socket, &QAbstractSocket::connected, timeout) };
            co_return result.has_value();
        }

        CoroTaskBool waitForDisconnected(int const timeout_msecs = 30'000) const
        { return waitForDisconnected(milliseconds{ timeout_msecs }); }

        CoroTaskBool waitForDisconnected(milliseconds const timeout) const {
            auto const socket { qobject_cast<QAbstractSocket *>(m_device_.data()) };
            if (socket->state() == QAbstractSocket::UnconnectedState) { co_return false; }
            const auto result{ co_await qCoro(socket, &QAbstractSocket::disconnected, timeout) };
            co_return result.has_value();
        }

        CoroTaskBool connectToHost(QString const & hostName, quint16 const port,
                             QIODevice::OpenMode const openMode = QIODevice::ReadWrite,
                             QAbstractSocket::NetworkLayerProtocol const protocol = QAbstractSocket::AnyIPProtocol,
                             milliseconds const timeout = std::chrono::seconds{30}) const
        {
            qobject_cast<QAbstractSocket *>(m_device_.data())->connectToHost(hostName, port, openMode, protocol);
            return waitForConnected(timeout);
        }

        CoroTaskBool connectToHost(QHostAddress const & address, quint16 const port,
                             QIODevice::OpenMode const openMode = QIODevice::ReadWrite,
                             milliseconds const timeout = std::chrono::seconds{30}) const
        {
            qobject_cast<QAbstractSocket *>(m_device_.data())->connectToHost(address, port, openMode);
            return waitForConnected(timeout);
        }

    private:
        CoroTaskOptionalBool waitForReadyReadImpl(milliseconds const timeout) const override {
            auto const socket { qobject_cast<QAbstractSocket *>(m_device_.data()) };
            if (socket->state() != QAbstractSocket::ConnectedState) { co_return false; }
            AbstractSocketReadySignalHelper helper { socket, &QIODevice::readyRead };
            co_return co_await qCoro(std::addressof(helper), qOverload<bool>(&WaitSignalHelper::ready), timeout);
        }

        CoroTaskOptionalQInt64 waitForBytesWrittenImpl(milliseconds const timeout) const override {
            auto const socket { qobject_cast<QAbstractSocket *>(m_device_.data()) };
            if (socket->state() != QAbstractSocket::ConnectedState) { co_return std::nullopt; }
            AbstractSocketReadySignalHelper helper { socket, &QIODevice::bytesWritten };
            co_return co_await qCoro(std::addressof(helper), qOverload<qint64>(&WaitSignalHelper::ready), timeout);
        }
    };

}

inline auto qCoro(QAbstractSocket & s) noexcept
{ return detail::QCoroAbstractSocket{s }; }

inline auto qCoro(QAbstractSocket * const s) noexcept
{ return detail::QCoroAbstractSocket{ s }; }

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
