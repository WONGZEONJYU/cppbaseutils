#ifndef XUTILS2_Q_CORO_QML_TASK_HPP
#define XUTILS2_Q_CORO_QML_TASK_HPP 1

#pragma once

#include <XCoroutine/xcoroutinetask.hpp>
#include <XQtHelper/qcoro/core/impl/waitfor.hpp>
#include <type_traits>
#include <optional>
#include <QJSValue>
#include <QJSEngine>
#include <QLoggingCategory>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
QT_WARNING_PUSH
QT_WARNING_DISABLE_MSVC(4458 4201)
#include <private/qjsvalue_p.h>
QT_WARNING_POP
#endif

Q_DECLARE_LOGGING_CATEGORY(qcoroqml)
Q_LOGGING_CATEGORY(qcoroqml, "qcoro.qml")

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace QmlPrivate {

    struct QmlTaskPrivate : QSharedData {
        std::optional<XCoroTask<QVariant>> m_task;
        constexpr QmlTaskPrivate() = default;
        explicit(false) QmlTaskPrivate(const QmlTaskPrivate &)
        { Q_UNREACHABLE(); }
    };
}

struct QmlTask {
    Q_GADGET
    QSharedDataPointer<QmlPrivate::QmlTaskPrivate> d{};

public:
    explicit QmlTask() noexcept
        :d { std::make_unique<QmlPrivate::QmlTaskPrivate>().release() }
    {

    }

    QmlTask(const QmlTask &other);
    QmlTask &operator=(const QmlTask &other);
    ~QmlTask();

    explicit(false) QmlTask(XCoroTask<QVariant> && task);

    template <typename T>
    explicit(false) QmlTask(XCoroTask<T> && task) : QmlTask {
        task.then([]<typename Tp>(Tp && result) -> XCoroTask<QVariant> {
            co_return QVariant::fromValue(std::forward<Tp>(result));
        }) }
    { qMetaTypeId<T>(); }

    template <typename T> requires (detail::TaskConvertible<T> && !std::is_same_v<T, QmlTask>)
    explicit(false) QmlTask(T && future) : QmlTask { detail::toTask(std::forward<T>(future)) }
    {   }

    template <typename = void>
    explicit(false) QmlTask(QCoro::Task<> &&task) : QmlTask(
        task.then([]() -> QCoro::Task<QVariant> {
            co_return QVariant();
        }))
    {   }

    Q_INVOKABLE void then(QJSValue func);

    Q_INVOKABLE QCoro::QmlTaskListener *await(const QVariant &intermediateValue = {});

};

class QmlTaskListener : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariant value READ value NOTIFY valueChanged)
    QVariant m_value{};

public:
    QVariant value() const;
    void setValue(QVariant &&value);
    Q_SIGNAL void valueChanged();
};




XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
