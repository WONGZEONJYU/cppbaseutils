#ifndef XUTILS2_Q_CORO_LOCAL_SOCKET_HPP
#define XUTILS2_Q_CORO_LOCAL_SOCKET_HPP 1

#include <XQtHelper/qcoro/core/qcoroiodevice.hpp>
#include <chrono>
#include <QLocalSocket>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    class SocketConnectedHelper : public QObject {
        Q_OBJECT
        QMetaObject::Connection m_signal_{},m_stateChange_{};

    public:
        using signal = void(QLocalSocket::*)();
        explicit(false) SocketConnectedHelper(const QLocalSocket * const local_socket, signal const signal )
            : m_signal_{ connect_(local_socket, signal, this, [this]{ emitReady(true); }) }
            , m_stateChange_{ connect_(local_socket, &QLocalSocket::stateChanged
                , [this]<typename Tp>(Tp && state){
                    if (std::forward<Tp>(state) == QLocalSocket::UnconnectedState) { emitReady(false); }
                } ) } {}

        Q_SIGNALS:
            void ready(bool);

    private:
        void emitReady(bool const result)
        { disconnect(m_signal_);disconnect(m_stateChange_); Q_EMIT ready(result); }

        template<typename ...Args>
        static QMetaObject::Connection connect_(Args && ...args)
        { return connect(std::forward<Args>(args)...); }
    };

    class LocalSocketReadySignalHelper : public WaitSignalHelper {
        Q_OBJECT
        QMetaObject::Connection m_stateChange_{};

    public:
        explicit(false) LocalSocketReadySignalHelper(const QLocalSocket * const socket, signalFunc<> const signal)
            : WaitSignalHelper{socket, signal}
            , m_stateChange_{ connect_(socket, &QLocalSocket::stateChanged, this,
                               [this]<typename Tp>(Tp && state){ stateChanged(std::forward<Tp>(state), false); })
            }
        {  }

        explicit(false) LocalSocketReadySignalHelper(const QLocalSocket * const socket, signalFunc<true> const signal)
            : WaitSignalHelper { socket, signal }
            , m_stateChange_{ connect_(socket, &QLocalSocket::stateChanged, this,
                                [this]<typename Tp>(Tp && state){ stateChanged(std::forward<Tp>(state), static_cast<qint64>(0)); })
            }
        {   }

    private:
        template<typename T>
        void stateChanged(QLocalSocket::LocalSocketState const state, T && result) {
            if (QLocalSocket::ConnectedState == state ) { return; }
            disconnect(m_stateChange_);
            emitReady(std::forward<T>(result));
        }
    };

    struct QCoroLocalSocket : QCoroIODevice {

        explicit(false) QCoroLocalSocket(QLocalSocket * const socket) noexcept
            : QCoroIODevice { socket }
        {}

        XCoroTask<bool> waitForConnected(int const timeout_msecs = 30'000) const
        { return waitForConnected(milliseconds{timeout_msecs}); }

        XCoroTask<bool> waitForConnected(milliseconds const timeout) const{
            auto const socket { qobject_cast<QLocalSocket *>(m_device_.data()) };
            if (QLocalSocket::ConnectedState == socket->state()) { co_return true; }
            SocketConnectedHelper helper(socket, &QLocalSocket::connected);
            auto const result { co_await qCoro(&helper, &SocketConnectedHelper::ready, timeout) };
            co_return result.value_or(false);
        }

        XCoroTask<bool> waitForDisconnected(int const timeout_msecs = 30'000) const
        { return waitForDisconnected(milliseconds{timeout_msecs}); }

        XCoroTask<bool> waitForDisconnected(milliseconds const timeout) const {
            auto const socket { qobject_cast<QLocalSocket *>(m_device_.data()) };
            if (QLocalSocket::UnconnectedState == socket->state() ) { co_return false; }
            auto const result{ co_await qCoro(socket, &QLocalSocket::disconnected, timeout) };
            co_return result.has_value();
        }

        XCoroTask<bool> connectToServer(QIODevice::OpenMode const openMode = QIODevice::ReadWrite,
                                   milliseconds const timeout = std::chrono::seconds{30}) const
        {
            qobject_cast<QLocalSocket *>(m_device_.data())->connectToServer(openMode);
            return waitForConnected(timeout);
        }

        XCoroTask<bool> connectToServer(QString const & name, QIODevice::OpenMode const openMode = QIODevice::ReadWrite,
                                   milliseconds const timeout = std::chrono::seconds{30}) const
        {
            qobject_cast<QLocalSocket *>(m_device_.data())->connectToServer(name, openMode);
            return waitForConnected(timeout);
        }

    private:
        XCoroTask<std::optional<bool>> waitForReadyReadImpl(milliseconds const timeout) const override {
            auto const socket { qobject_cast<QLocalSocket *>(m_device_.data()) };
            if (QLocalSocket::ConnectedState != socket->state() ) { co_return false; }
            LocalSocketReadySignalHelper helper {socket, &QLocalSocket::readyRead };
            co_return co_await qCoro(std::addressof(helper), qOverload<bool>(&LocalSocketReadySignalHelper::ready), timeout);
        }

        XCoroTask<std::optional<qint64>> waitForBytesWrittenImpl(milliseconds const timeout) const override {
            auto const socket { qobject_cast<QLocalSocket *>(m_device_.data()) };
            if (socket->state() != QLocalSocket::ConnectedState) { co_return std::nullopt; }
            LocalSocketReadySignalHelper helper { socket, &QLocalSocket::bytesWritten };
            co_return co_await qCoro(std::addressof(helper), qOverload<qint64>(&LocalSocketReadySignalHelper::ready), timeout);
        }
    };

}

inline auto qCoro(QLocalSocket & s) noexcept
{ return detail::QCoroLocalSocket {std::addressof(s)}; }

inline auto qCoro(QLocalSocket * const s) noexcept
{ return detail::QCoroLocalSocket {s}; }

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
