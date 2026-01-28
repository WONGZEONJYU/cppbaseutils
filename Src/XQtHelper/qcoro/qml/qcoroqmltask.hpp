#ifndef XUTILS2_Q_CORO_QML_TASK_HPP
#define XUTILS2_Q_CORO_QML_TASK_HPP 1

#pragma once

#include <XCoroutine/xcoroutinetask.hpp>
#include <XQtHelper/qcoro/core/waitfor.hpp>
#include <type_traits>
#include <optional>
#include <QObject>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
QT_WARNING_PUSH
QT_WARNING_DISABLE_MSVC(4458 4201)
#include <private/qjsvalue_p.h>
QT_WARNING_POP
#endif

inline Q_DECLARE_LOGGING_CATEGORY(qcoroqml)
inline Q_LOGGING_CATEGORY(qcoroqml, "qcoro.qml")

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace QmlPrivate {

    struct QmlTaskPrivate : QSharedData {
        std::optional<XCoroTask<QVariant>> m_task;
        constexpr QmlTaskPrivate() = default;
        QmlTaskPrivate(QmlTaskPrivate const &)
        { Q_UNREACHABLE(); }
    };

    inline QJSEngine *getEngineForValue(QJSValue const & val) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        // QJSValue::engine is deprecated, but still nicer, since it doesn't require using private symbols
        QT_WARNING_PUSH
        QT_WARNING_DISABLE_DEPRECATED
        return val.engine();
        QT_WARNING_POP
    #else
        return QJSValuePrivate::engine(std::addressof(val))->jsEngine();
#endif
    }

}

class QmlTaskListener : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariant value READ value NOTIFY valueChanged)
    QVariant m_value_{};

public:
    using QObject::QObject;
    [[nodiscard]] QVariant value() const { return m_value_; }
    void setValue(QVariant && value) noexcept
    { m_value_ = std::move(value); Q_EMIT valueChanged(); }
    Q_SIGNAL void valueChanged();
};

struct QmlTask {
    Q_GADGET
    QSharedDataPointer<QmlPrivate::QmlTaskPrivate> m_d_{};

public:
    Q_IMPLICIT QmlTask() noexcept
        : m_d_ { std::make_unique<QmlPrivate::QmlTaskPrivate>().release() }
    {   }

    X_DEFAULT_COPY(QmlTask)

    ~QmlTask() = default;

    Q_IMPLICIT QmlTask(XCoroTask<QVariant> && task)
        : QmlTask {}
    { m_d_->m_task = std::move(task); }

    template <typename T>
    Q_IMPLICIT QmlTask(XCoroTask<T> && task)
        : QmlTask { task.then([]<typename Tp>(Tp && result) -> XCoroTask<QVariant> {
            co_return QVariant::fromValue(std::forward<Tp>(result));
        }) }
    { qMetaTypeId<T>(); }

    template <typename T> requires (detail::TaskConvertible<T> && !std::is_same_v<T, QmlTask>)
    Q_IMPLICIT QmlTask(T && future) : QmlTask { detail::toTask(std::forward<T>(future)) }
    {   }

    template <typename T = void>
    Q_IMPLICIT QmlTask(XCoroTask<> && task)
        : QmlTask { task.then([]()-> XCoroTask<QVariant> { co_return QVariant{ }; }) }
    { using type [[maybe_unused]] = T; }

    Q_INVOKABLE void then(QJSValue func) {
        if (!m_d_->m_task) {
            qCWarning(qcoroqml, ".then called on a QmlTask that is not connected to any coroutine. "
                                "Make sure you don't default-construct QmlTask in your code");
            return;
        }

        if (!func.isCallable()) {
            qCWarning(qcoroqml, ".then called with an argument that is not a function. The .then call will do nothing");
            return;
        }

        m_d_->m_task->then([func = std::move(func)](QVariant const & result){
            auto const jSval{ QmlPrivate::getEngineForValue(func)->toScriptValue(result)};
            [[maybe_unused]] auto const _{ func.call({jSval}) };
        });
    }

    Q_INVOKABLE QmlTaskListener * await(QVariant const & intermediateValue = {}) {
        QPointer listener { std::make_unique<QmlTaskListener>().release() };
        if (!intermediateValue.isNull()) { listener->setValue(QVariant(intermediateValue)); }
        m_d_->m_task->then([listener]<typename Tp>(Tp && value){
            if (listener) { listener->setValue(std::forward<Tp>(value)); }
        });
        return listener.data();
    }
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
