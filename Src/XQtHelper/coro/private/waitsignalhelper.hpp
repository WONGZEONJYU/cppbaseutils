#ifndef XUTILS2_WAIT_SIGNAL_HELPER_HPP
#define XUTILS2_WAIT_SIGNAL_HELPER_HPP 1

#pragma once

#include <XGlobal/xversion.hpp>
#include <QIODevice>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    class WaitSignalHelper : public QObject {
        Q_OBJECT
    protected:
        QMetaObject::Connection m_ready_ {},m_aboutToClose_ { };

    public:
        using signalFunc = void(QIODevice::*)();
        explicit WaitSignalHelper(QIODevice * const device,signalFunc const f )
            : m_ready_ { connect(device, f, [this]{ this->emitReady(true); }) }
            , m_aboutToClose_ { connect(device, &QIODevice::aboutToClose, [this]{ this->emitReady(false); }) }
        { }

        using signalFuncArgs = void(QIODevice::*)(qint64);
        explicit WaitSignalHelper(QIODevice * const device, signalFuncArgs const f)
            : m_ready_ { connect(device, f, this, &WaitSignalHelper::emitReady<qint64>) }
            , m_aboutToClose_ { connect(device, &QIODevice::aboutToClose, [this]{ this->emitReady(static_cast<qint64>(0)); }) }
        { }

    Q_SIGNALS:
        void ready(bool result);
        void ready(qint64 result);

    protected:
        template<typename T>
        void emitReady(T const result) { cleanup(); Q_EMIT this->ready(result); }
        virtual void cleanup() { disconnect(mReady); disconnect(mAboutToClose); }
    };

}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
