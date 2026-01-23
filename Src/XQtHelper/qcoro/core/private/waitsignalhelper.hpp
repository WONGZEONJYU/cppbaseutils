#ifndef XUTILS2_WAIT_SIGNAL_HELPER_HPP
#define XUTILS2_WAIT_SIGNAL_HELPER_HPP 1

#pragma once

#include <XGlobal/xversion.hpp>
#include <QIODevice>
#include <type_traits>

#ifndef Q_OBJECT
    #define Q_OBJECT
#endif

#ifndef Q_SIGNALS
    #define Q_SIGNALS public
#endif

#ifndef Q_EMIT
    #define Q_EMIT
#endif

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    class WaitSignalHelper : public QObject {
        Q_OBJECT
    protected:
        QMetaObject::Connection m_ready_ {},m_aboutToClose_ {};

    public:
        template<bool b = false>
        using signalFunc = std::conditional_t<b,void(QIODevice::*)(qint64),void(QIODevice::*)()>;

        explicit(false) WaitSignalHelper(const QIODevice * const device , signalFunc<> const signalFunc)
            : m_ready_ { connect_(device, signalFunc, this,[this]{ emitReady(true); }) }
            , m_aboutToClose_ { connect_(device, &QIODevice::aboutToClose, this ,[this]{ emitReady(false); }) }
        {   }

        explicit(false) WaitSignalHelper(const QIODevice * const device, signalFunc<true> const signalFunc)
            : m_ready_ { connect_(device, signalFunc, this, &WaitSignalHelper::emitReady<qint64>) }
            , m_aboutToClose_ { connect_(device, &QIODevice::aboutToClose, this,[this]{ emitReady( qint64{} ); }) }
        {   }

        ~WaitSignalHelper() override = default;

    Q_SIGNALS:
        void ready(bool);
        void ready(qint64);

    protected:
        template<typename T> void emitReady(T const result)
        { cleanup(); Q_EMIT this->ready(result); }

        virtual void cleanup()
        { disconnect(m_ready_); disconnect(m_aboutToClose_); }

        template<typename ...Args> static QMetaObject::Connection connect_(Args && ...args)
        { return QObject::connect(std::forward<Args>(args)...); }
    };

}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
