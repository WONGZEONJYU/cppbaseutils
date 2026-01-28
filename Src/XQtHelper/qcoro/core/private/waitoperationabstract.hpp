#ifndef XUTILS2_WAIT_OPERATION_ABSTRACT_P_HPP
#define XUTILS2_WAIT_OPERATION_ABSTRACT_P_HPP 1

#pragma once

#include <XGlobal/xclasshelpermacros.hpp>
#include <XGlobal/xversion.hpp>
#include <coroutine>
#include <memory>
#include <QTimer>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    template<typename T>
    class WaitOperationAbstract {
    protected:
        QPointer<T> m_QObject_ {};
        std::unique_ptr<QTimer> m_timeoutTimer_ {};
        QMetaObject::Connection m_conn_ {};
        bool m_timedOut_ {};

    public:
        Q_DISABLE_COPY(WaitOperationAbstract)
        X_DEFAULT_MOVE(WaitOperationAbstract)

        virtual ~WaitOperationAbstract() = default;

        [[nodiscard]] constexpr bool await_resume() const noexcept
        { return !m_timedOut_; }

    protected:
        Q_IMPLICIT constexpr WaitOperationAbstract(T * const obj, int const timeout_msecs)
            : m_QObject_ { obj }
        {
            if (timeout_msecs < 0) { return; }
            m_timeoutTimer_ = std::make_unique<QTimer>();
            m_timeoutTimer_->setInterval(timeout_msecs);
            m_timeoutTimer_->setSingleShot(true);
        }

        Q_IMPLICIT constexpr WaitOperationAbstract(T & obj, int const timeout_msecs)
            : WaitOperationAbstract { std::addressof(obj),timeout_msecs }
        {   }

        void startTimeoutTimer(std::coroutine_handle<> const h) {
            if (!m_timeoutTimer_) { return; }
            m_timeoutTimer_->callOnTimeout([this,h]{ m_timedOut_ = true; resume(h); });
            m_timeoutTimer_->start();
        }

        void resume(std::coroutine_handle<> const h) {
            if (m_timeoutTimer_) { m_timeoutTimer_->stop(); }
            QObject::disconnect(m_conn_);
            QTimer::singleShot(0, [h]{ h.resume(); });
        }
    };

}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
