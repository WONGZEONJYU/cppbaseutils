#ifndef XUTILS2_Q_CORO_QML_HPP
#define XUTILS2_Q_CORO_QML_HPP 1

#pragma once
#include <XQtHelper/qcoro/qml/qcoroqmltask.hpp>
#include <QQmlApplicationEngine>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace Qml{
    inline void registerTypes() {
        qRegisterMetaType<QmlTask>();
        qmlRegisterAnonymousType<QmlTaskListener>("QCoro", 0);
    }
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
