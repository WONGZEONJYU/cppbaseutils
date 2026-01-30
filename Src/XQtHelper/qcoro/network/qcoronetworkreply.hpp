#ifndef XUTILS2_Q_CORO_NETWORK_REPLY_HPP
#define XUTILS2_Q_CORO_NETWORK_REPLY_HPP 1

#pragma once

#include <XQtHelper/qcoro/core/qcoroiodevice.hpp>
#include <QNetworkReply>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    struct ReplyWaitSignalHelper : WaitSignalHelper {
        Q_OBJECT
        QMetaObject::Connection m_error_{},m_finished_{};
    public:
        Q_IMPLICIT ReplyWaitSignalHelper(QNetworkReply const * const reply, signalFunc<> const signal)
            : WaitSignalHelper { reply, signal }
            , m_error_{ connect_(reply,&QNetworkReply::errorOccurred,this, [this]{ emitReady(false); },Qt::QueuedConnection) }
            , m_finished_{ connect_(reply,&QNetworkReply::finished,this, [this]{ emitReady(true); },Qt::QueuedConnection) }
        {   }

        Q_IMPLICIT ReplyWaitSignalHelper(QNetworkReply const & reply, signalFunc<> const signal)
            : ReplyWaitSignalHelper { std::addressof(reply) , signal }
        {   }

        Q_IMPLICIT ReplyWaitSignalHelper(QNetworkReply const * const reply, signalFunc<qint64> const signal)
            : WaitSignalHelper { reply, signal }
            , m_error_{ connect_(reply, &QNetworkReply::errorOccurred,this,[this]{ emitReady(0LL); },Qt::QueuedConnection) }
            , m_finished_{ connect_(reply, &QNetworkReply::finished,this, [this]{ emitReady(0LL); },Qt::QueuedConnection) }
        {   }

        Q_IMPLICIT ReplyWaitSignalHelper(QNetworkReply const & reply, signalFunc<qint64> const signal)
            : ReplyWaitSignalHelper { std::addressof(reply) , signal }
        {   }

    private:
        void cleanup() override
        { disconnect(m_error_); disconnect(m_finished_); WaitSignalHelper::cleanup(); }
    };

    class QCoroNetworkReply final : public QCoroIODevice {

        class WaitForFinishedOperation final {

            struct Private {
                static_assert(sizeof(std::unique_ptr<Private>)  + sizeof(void*) == sizeof(QPointer<QNetworkReply>),
                    "QCoroNetworkReply::WaitForFinishedOperation is not BC with previous version, see comment in header");

                QPointer<QNetworkReply> m_reply{};
                QObject m_dummy{};

                Q_IMPLICIT Private(QPointer<QNetworkReply> const & reply)
                    : m_reply { reply }
                { if (reply) { m_dummy.moveToThread(reply->thread()); } }
            };

            std::unique_ptr<Private> m_d_{};
            // BC: This class used to have a QPointer<QNetworkReply>, which is 2*sizeof(void*),
            // while std::unique_ptr is only sizeof(void*), so this dummy one is to ensure binary
            // compatibility of this class.
            // FIXME: Remove in 1.0
            // Silly gcc 11, cannot detect the dummy is unused and warns about unused attribute
            #if __GNUC__ > 11 || defined(__clang__) || defined(_MSC_VER)
            [[maybe_unused]]
            #endif
            void * m_dummy_ {};

        public:
            X_IMPLICIT WaitForFinishedOperation(QPointer<QNetworkReply> const & reply)
                : m_d_ { std::make_unique<Private>(reply) }
            {   }

            X_IMPLICIT WaitForFinishedOperation(QPointer<QNetworkReply> const * const reply)
                : WaitForFinishedOperation { *reply }
            {   }

            ~WaitForFinishedOperation() = default;

            [[nodiscard]] bool await_ready() const noexcept
            { return !m_d_->m_reply || m_d_->m_reply->isFinished(); }

            void await_suspend(std::coroutine_handle<> const h) const {
                if (!m_d_->m_reply) { h.resume(); return; }
                QObject::connect(m_d_->m_reply.data(),&QNetworkReply::finished,std::addressof(m_d_->m_dummy),
                    [h]{ h.resume(); },Qt::QueuedConnection
                );
            }

            [[nodiscard]] QNetworkReply * await_resume() const noexcept
            { return m_d_->m_reply.data(); }
        };

    public:
        using QCoroIODevice::QCoroIODevice;

        TaskBool waitForFinished(milliseconds const timeout = milliseconds{-1}) const {
            auto const reply { qobject_cast<QNetworkReply *>(m_device_.data()) };
            if (reply->isFinished()) { co_return true; }
            auto const result{ co_await qCoro(reply, &QNetworkReply::finished, timeout) };
            co_return result.has_value();
        }

    private:
        TaskOptionalBool waitForReadyReadImpl(milliseconds const timeout) const override {
            auto const reply { qobject_cast<QNetworkReply *>(m_device_.data()) };
            if (reply->isFinished()) { co_return true; }
            ReplyWaitSignalHelper helper { reply, &QNetworkReply::readyRead };
            co_return co_await qCoro(std::addressof(helper), qOverload<bool>(&ReplyWaitSignalHelper::ready), timeout);
        }

        TaskOptionalQInt64 waitForBytesWrittenImpl(milliseconds const timeout) const override {
            auto const reply { qobject_cast<QNetworkReply *>(m_device_.data()) };
            if (reply->isFinished()) { co_return false; }
            ReplyWaitSignalHelper helper { reply, &QNetworkReply::bytesWritten };
            co_return co_await qCoro(std::addressof(helper), qOverload<qint64>(&ReplyWaitSignalHelper::ready), timeout);
        }

        template<typename > friend struct awaiter_type;
    };

    template<> struct awaiter_type<QNetworkReply>
    { using type = QCoroNetworkReply::WaitForFinishedOperation; };

    template<> struct awaiter_type<QNetworkReply *>
    { using type = QCoroNetworkReply::WaitForFinishedOperation; };

}

inline auto qCoro(QNetworkReply & s) noexcept
{ return detail::QCoroNetworkReply { s }; }

inline auto qCoro(QNetworkReply * const s) noexcept
{ return detail::QCoroNetworkReply { s }; }

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
