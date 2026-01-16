#ifndef XUTILS2_Q_CORO_IO_DEVICE_HPP
#define XUTILS2_Q_CORO_IO_DEVICE_HPP 1

#include <XCoroutine/xcoroutinetask.hpp>
#include <XQtHelper/qcoro/core/private/waitoperationabstract_p.hpp>
#include <XQtHelper/qcoro/core/private/waitsignalhelper.hpp>
#include <XQtHelper/qcoro/core/qcorosignal.hpp>
#include <optional>
#include <functional>
#include <chrono>
#include <QPointer>
#include <QByteArray>
#include <QIODevice>
#include <QTimer>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    class QCoroIODevice {
        template<typename> friend struct awaiter_type;
    protected:
        QPointer<QIODevice> m_device_ {};

    private:
        struct OperationAbstract {
        protected:
            QPointer<QIODevice> m_device_ {};
            QMetaObject::Connection m_conn_ {}, m_closeConn_ {}, m_finishedConn_ {};

        public:
            Q_DISABLE_COPY(OperationAbstract);
            X_DEFAULT_MOVE(OperationAbstract)

            virtual ~OperationAbstract() = default;

        protected:
            explicit(false) OperationAbstract(QIODevice * const device)
                : m_device_ { device } {  }

            virtual void finish(std::coroutine_handle<> const h) {
                QObject::disconnect(m_conn_);
                QObject::disconnect(m_closeConn_);
                // Delayed trigger
                QTimer::singleShot(0, [h]{ h.resume(); });
            }
        };

    protected:
        struct ReadOperation : OperationAbstract {
            using callback_t = std::function<QByteArray(QIODevice *)>;
        private:
            using Base = OperationAbstract;
            callback_t m_resultCb_{};

        public:
            explicit(false) ReadOperation(QIODevice * const device, callback_t && resultCb)
                : Base { device } , m_resultCb_ { std::move(resultCb) } { }

            Q_DISABLE_COPY(ReadOperation);
            X_DEFAULT_MOVE(ReadOperation)

            [[nodiscard]] virtual bool await_ready() const noexcept
            { return !m_device_ || !m_device_->isOpen() || !m_device_->isReadable() || m_device_->bytesAvailable() > 0; }

            virtual void await_suspend(std::coroutine_handle<> const h) noexcept {
                Q_ASSERT(m_device_);
                auto const slotF { [this, h]() noexcept{ finish(h); } };
                m_conn_ = QObject::connect(m_device_, &QIODevice::readyRead,slotF);
                m_closeConn_ = QObject::connect(m_device_, &QIODevice::aboutToClose,slotF);
            }

            [[nodiscard]] QByteArray await_resume() const { return m_resultCb_(m_device_); }
        };

        struct ReadAllOperation final : ReadOperation {
            explicit(false) ReadAllOperation(QIODevice * const device)
                : ReadOperation { device,[](QIODevice * const d){ return d->readAll(); } } { }

            explicit(false) ReadAllOperation(QIODevice & device)
                : ReadAllOperation { std::addressof(device) } { }
        };

    public:
        explicit(false) QCoroIODevice(QIODevice * const device)
            : m_device_ { device } {}

        virtual ~QCoroIODevice() = default;

        using milliseconds = std::chrono::milliseconds;

        XCoroTask<QByteArray> readAll(milliseconds const timeout = milliseconds{-1}) {
            auto const d{ m_device_ };
            if (!co_await waitForReadyRead(timeout)) { co_return QByteArray{}; }
            co_return d->readAll();
        }

        XCoroTask<QByteArray> read(qint64 const maxSize, milliseconds const timeout = milliseconds{-1}) {
            auto const d{ m_device_ };
            if (!co_await waitForReadyRead(timeout)) { co_return QByteArray{}; }
            co_return d->read(maxSize);
        }

        XCoroTask<QByteArray> readLine(qint64 const maxSize = {} ,milliseconds const timeout = milliseconds{-1}) {
            auto const d{ m_device_ };
            if (!co_await waitForReadyRead(timeout)) { co_return QByteArray {}; }
            co_return d->readLine(maxSize);
        }

        XCoroTask<qint64> write(QByteArray const & buffer) {
            qint64 bytesConfirmed {};
            auto const bytesWritten{ m_device_->write(buffer) };
            while (bytesConfirmed < bytesWritten) {
                auto const flushed{ co_await waitForBytesWritten(-1) };
                // There was an intermediate error and we don't know how much was actually
                // written, so we report only what we know for sure was written.
                if (!flushed.has_value()) { break; }
                bytesConfirmed += *flushed;
            }
            co_return bytesConfirmed;
        }

        XCoroTask<bool> waitForReadyRead(milliseconds const timeout) {
            if (!m_device_->isReadable()) { co_return false; }
            if (m_device_->bytesAvailable() > 0) { co_return true; }
            auto const result{ co_await waitForReadyReadImpl(timeout) };
            co_return result.has_value();
        }

        XCoroTask<bool> waitForReadyRead(int const timeout_msecs)
        { return waitForReadyRead(milliseconds{timeout_msecs}); }

        XCoroTask<std::optional<qint64>> waitForBytesWritten(milliseconds const timeout) {
            if (!m_device_->isWritable()) { co_return std::nullopt; }
            if (!m_device_->bytesToWrite()) { co_return 0; }
            auto const result { co_await waitForBytesWrittenImpl(timeout) };
            co_return result;
        }

        XCoroTask<std::optional<qint64>> waitForBytesWritten(int const timeout_msecs)
        { return waitForBytesWritten(milliseconds{timeout_msecs}); }

    protected:
        virtual XCoroTask<std::optional<bool>> waitForReadyReadImpl(milliseconds const timeout) {
            WaitSignalHelper helper {m_device_.data(), &QIODevice::readyRead };
            co_return co_await qCoro(std::addressof(helper), qOverload<bool>(&WaitSignalHelper::ready), timeout);
        }

        virtual XCoroTask<std::optional<qint64>> waitForBytesWrittenImpl(milliseconds const timeout) {
            WaitSignalHelper helper {m_device_.data(), &QIODevice::bytesWritten };
            co_return co_await qCoro(std::addressof(helper), qOverload<qint64>(&WaitSignalHelper::ready), timeout);
        }
    };

    template<typename T> requires std::is_base_of_v<QIODevice, T>
    struct awaiter_type<T> { using type = QCoroIODevice::ReadAllOperation; };

    template<typename T> requires std::is_base_of_v<QIODevice, T>
    struct awaiter_type<T *> { using type = QCoroIODevice::ReadAllOperation; };

}

inline auto qCoro(QIODevice & d) noexcept
{ return detail::QCoroIODevice {std::addressof(d) }; }

inline auto qCoro(QIODevice * d) noexcept
{ return detail::QCoroIODevice{d}; }

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

class QAbstractSocket;
class QLocalSocket;
class QNetworkReply;

/*
If you got here due to a compile error, make sure to #include the QCoro header
for the corresponding class, so that the qCoro() overload that wraps the QIODevice-derived
classes in their respective QCoroIODevice-derived wrappers is used.

Wrapping those classes directly into QCoroIODevice will cause co_awaiting certain operations to not
work as expected.
*/

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

auto qCoro(QAbstractSocket *) noexcept; // You are likely missing "#include <QCoroAbstractSocket>"
auto qCoro(QLocalSocket *) noexcept; // You are likely missing "#include <QCoroLocalSocket>"
auto qCoro(QNetworkReply *) noexcept; // You are likely missing "#include <QNetworkReply>"

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
