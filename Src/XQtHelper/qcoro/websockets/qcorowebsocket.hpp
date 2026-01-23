#ifndef XUTILS2_Q_CORO_WEBSOCKET_HPP
#define XUTILS2_Q_CORO_WEBSOCKET_HPP 1

#pragma once

#include <XQtHelper/qcoro/core/qcorosignal.hpp>
#include <QWebSocket>
#include <QDebug>

using TupleQInt64QByteArray = std::tuple<qint64, QByteArray>;
using TupleQByteArrayBool = std::tuple<QByteArray, bool>;
using TupleQStringBool = std::tuple<QString, bool>;
using TupleQByteArray = std::tuple<QByteArray>;
using TupleQString = std::tuple<QString>;

Q_DECLARE_METATYPE(std::optional<TupleQInt64QByteArray>)
Q_DECLARE_METATYPE(std::optional<TupleQByteArrayBool>)
Q_DECLARE_METATYPE(std::optional<std::tuple<QByteArray>>)
Q_DECLARE_METATYPE(std::optional<TupleQStringBool>)
Q_DECLARE_METATYPE(std::optional<std::tuple<QString>>)

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    class WebSocketStateWatcher : public QObject {
        Q_OBJECT
        QMetaObject::Connection m_state_{},m_error_{};

    public:
        explicit(false) WebSocketStateWatcher(const QWebSocket * const socket, QAbstractSocket::SocketState const desiredState)
            : m_state_ { connect(socket,&QWebSocket::stateChanged,this,
                [this, desiredState]<typename Tp>(Tp && newState){
                if (std::forward<Tp>(newState) == desiredState) { emitReady(true); }
            }) }
            , m_error_ {connect(socket, qOverload<QAbstractSocket::SocketError>(
    #if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
                                         &QWebSocket::errorOccurred
    #else
                                         &QWebSocket::error
    #endif
                                         ), this,[this]<typename Tp>(Tp && error) {
                qWarning() << "QWebSocket failed to connect to a websocket server: " << std::forward<Tp>(error);
                emitReady(false);
            }) }
        {   }

    Q_SIGNALS:
        void ready(bool);

    private:
        void emitReady(bool const result) {
            disconnect(m_state_);
            disconnect(m_error_);
            Q_EMIT ready(result);
        }
    };

    template<typename> struct signal_args;

    template<typename T, typename R, typename ... Args>
    struct signal_args<R(T::*)(Args ...)> { using types = std::tuple<std::remove_cvref_t<Args> ...>; };

    template<typename T> using signal_args_t = signal_args<T>::types;

    template<typename> struct unwrapped_signal_args;

    template<typename ... Args>
    struct unwrapped_signal_args<std::tuple<Args ...>> {
    private:
        using args_tuple = std::tuple<std::remove_cvref_t<Args> ...>;
    public:
        using type = std::conditional_t<std::tuple_size_v<args_tuple> == 1,
                                        std::tuple_element_t<0, args_tuple>,
                                        args_tuple>;
    };

    template<typename ... Args>
    using unwrapped_signal_args_t = unwrapped_signal_args<Args ...>::type;

    class WebSocketSignalWatcher : public QObject {
        Q_OBJECT
    public:
        template<typename Signal>
        explicit(false) WebSocketSignalWatcher(QWebSocket * const socket, Signal const signal) {

            qRegisterMetaType< std::optional< TupleQInt64QByteArray > >();
            qRegisterMetaType< std::optional< TupleQByteArrayBool> >();
            qRegisterMetaType< std::optional< TupleQByteArray > >();
            qRegisterMetaType< std::optional< TupleQStringBool > >();
            qRegisterMetaType< std::optional< TupleQString > >();

            connect(socket, signal, this, [this]<typename ...Args>(Args && ... args)
                { Q_EMIT this->ready(std::make_optional(std::make_tuple(std::forward<Args>(args) ...))); }
            );

            connect(socket, &QWebSocket::stateChanged, this,
                [this]<typename Tp>(Tp && state) {
                    // In theory, WebSocketSignalWatcher should never be used on
                    // unconnected socket, so maybe the check is redundant
                    if ( QAbstractSocket::ConnectedState != std::forward<Tp>(state) )
                    { Q_EMIT this->ready(std::optional<signal_args_t<Signal>>{}); }
                }
            );
        }

    Q_SIGNALS:
        void ready(std::optional<TupleQInt64QByteArray>); // ping
        void ready(std::optional<TupleQByteArrayBool>); // binary frames
        void ready(std::optional<TupleQByteArray>); // binary messages
        void ready(std::optional<TupleQStringBool>); // text frames
        void ready(std::optional<TupleQString>); // text messages
    };

    using milliseconds = std::chrono::milliseconds;

    template<typename Signal>
    static auto watcherGenerator(QWebSocket * const ws, Signal const signal, milliseconds const timeout) ->
        XAsyncGenerator<unwrapped_signal_args_t<signal_args_t<Signal>>>
    {
        WebSocketSignalWatcher watcher { ws, signal };
        using signalType = std::optional<signal_args_t<Signal>>;
        auto signalListener {
            qCoroSignalListener(std::addressof(watcher), qOverload<signalType>(&WebSocketSignalWatcher::ready), timeout)
        };

        for (auto it { co_await signalListener.begin() };
            it != signalListener.end();co_await ++it)
        {
            if (!it->has_value()) { break; }
            // If the signal is a single-value tuple, we unwrap it from the tuple, otherwise we yield the whole tuple.
            if constexpr (1 == std::tuple_size_v<typename signalType::value_type>)
            { co_yield std::get<0>(**it); }
            else { co_yield **it; }
        }
    }

    class QCoroWebSocket {
        QWebSocket * m_webSocket_{};
    public:
        explicit(false) QCoroWebSocket(QWebSocket * const websocket)
            : m_webSocket_ { websocket } {   }

        using milliseconds = std::chrono::milliseconds;
        using CoroTaskBool = XCoroTask<bool>;

        [[nodiscard]] CoroTaskBool open(QUrl const & url, milliseconds const timeout = milliseconds{-1}) const
        { return openHelper(url, timeout); }

        [[nodiscard]] CoroTaskBool open(QNetworkRequest const & request, milliseconds const timeout = milliseconds{-1}) const
        { return openHelper(request, timeout); }

        [[nodiscard]] XCoroTask<std::optional<milliseconds>> ping(QByteArray const & payload, milliseconds const timeout = milliseconds{-1}) const {
            if (QAbstractSocket::ConnectedState != m_webSocket_->state() ) { co_return std::nullopt; }
            WebSocketSignalWatcher watcher { m_webSocket_, &QWebSocket::pong };
            m_webSocket_->ping(payload);
            auto const result { co_await qCoro(std::addressof(watcher)
                , qOverload< std::optional< TupleQInt64QByteArray > >(&WebSocketSignalWatcher::ready), timeout) };
            if (result.has_value() && result->has_value()) { co_return milliseconds{std::get<0>(**result)}; }
            co_return std::nullopt;
        }

        [[nodiscard]] XAsyncGenerator<TupleQByteArrayBool> binaryFrames(milliseconds const timeout = milliseconds{-1}) const
        { return watcherGenerator(m_webSocket_, &QWebSocket::binaryFrameReceived, timeout); }

        [[nodiscard]] XAsyncGenerator<QByteArray> binaryMessages(milliseconds const timeout = milliseconds{-1}) const
        { return watcherGenerator(m_webSocket_, &QWebSocket::binaryMessageReceived, timeout); }

        [[nodiscard]] XAsyncGenerator<TupleQStringBool> textFrames(milliseconds const timeout = milliseconds{-1}) const
        { return watcherGenerator(m_webSocket_, &QWebSocket::textFrameReceived, timeout); }

        [[nodiscard]] XAsyncGenerator<QString> textMessages(milliseconds const timeout = milliseconds{-1}) const
        { return watcherGenerator(m_webSocket_, &QWebSocket::textMessageReceived, timeout); }

    private:
        template<typename U> // 如果CoroTaskBool 是惰性协程,这样设计会可能有bug,非惰性没有问题
        [[nodiscard]] CoroTaskBool openHelper(U && u, milliseconds const timeout = milliseconds{-1}) const {
            if (QAbstractSocket::ConnectedState == m_webSocket_->state()) { co_return true; }
            WebSocketStateWatcher watcher { m_webSocket_, QAbstractSocket::ConnectedState };
            m_webSocket_->open(std::forward<U>(u));
            auto const result { co_await qCoro(std::addressof(watcher), &WebSocketStateWatcher::ready, timeout)};
            co_return result.value_or(false);
        }
    };

}

auto inline qCoro(QWebSocket * const websocket) noexcept
{ return detail::QCoroWebSocket{websocket}; }

auto inline qCoro(QWebSocket & websocket) noexcept
{ return detail::QCoroWebSocket{std::addressof(websocket)}; }

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
