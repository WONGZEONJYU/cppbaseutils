#include <XQtHelper/qcoro/qml/qcoroqml.hpp>
#include <XQtHelper/qcoro/core/qcorotimer.hpp>
#include <XQtHelper/qcoro/qml/qcoroqmltask.hpp>
#include <XQtHelper/qcoro/core/qcorofuture.hpp>

#include <QTest>
#include <QTimer>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <chrono>

struct QmlObject : QObject {
    Q_OBJECT
    int m_numTestsPassed_ {};

public:
    Q_INVOKABLE XUtils::QmlTask startTimer() {
        auto const timer { std::make_unique<QTimer>(this).release()};
        timer->setSingleShot(true);
        using namespace std::chrono_literals;
        timer->start(1s);
        return [timer]() -> XUtils::XCoroTask<> {
            co_await timer;
        }();
    }

    Q_INVOKABLE XUtils::QmlTask qmlTaskFromTimer() {
        auto const timer { std::make_unique<QTimer>(this).release() };
        timer->setSingleShot(true);
        using namespace std::chrono_literals;
        timer->start(1s);
        return timer;
    }

    Q_INVOKABLE XUtils::QmlTask qmlTaskFromFuture() {
        QFutureInterface<QString> interface{};
        interface.reportResult(QStringLiteral("Success"));
        interface.reportFinished();
        return interface.future();
    }

    Q_INVOKABLE void reportTestSuccess() {
        if (4 == ++m_numTestsPassed_ ) { Q_EMIT success(); }
        // Number of java script functions that call reportTestSuccess
    }

    Q_SIGNAL void success();
};

struct QCoroQmlTaskTest : QObject {
    Q_OBJECT

    Q_SLOT void testQmlCallback() {
        QQmlApplicationEngine engine;
        qmlRegisterSingletonType<QmlObject>("qcoro.test", 0, 1, "QmlObject", [](QQmlEngine *, QJSEngine *) {
            return new QmlObject();
        });

        XUtils::Qml::registerTypes();

        engine.loadData(R"(
import qcoro.test 0.1
import QCoro 0
import QtQuick 2.7

QtObject {
    property string value: QmlObject.qmlTaskFromFuture().await("Loading...").value

    property string valueWithoutIntermediate: QmlObject.qmlTaskFromFuture().await().value

    onValueChanged: {
        if (value == "Success") {
            console.log("awaiting finished")
            QmlObject.reportTestSuccess()
        }
    }

    Component.onCompleted: {
        QmlObject.startTimer().then(() => {
            console.log("QCoro::Task JavaScript callback called")
            QmlObject.reportTestSuccess()
        })

        QmlObject.qmlTaskFromTimer().then(() => {
            console.log("QTimer JavaScript callback called")
            QmlObject.reportTestSuccess()
        })

        QmlObject.qmlTaskFromFuture().then(() => {
            console.log("QFuture JavaScript callback called")
            QmlObject.reportTestSuccess()
        })
    }
}
)");
        auto const object { engine.singletonInstance<QmlObject *>(qmlTypeId("qcoro.test", 0, 1, "QmlObject"))};

        auto const timeout { std::make_unique<QTimer>(this).release() };
        timeout->setSingleShot(true);
        using namespace std::chrono_literals;
        timeout->setInterval(2s);
        timeout->start();

        auto running { true };
        // End the event loop normally
        connect(object, &QmlObject::success, this, [&] {
            timeout->stop();
            running = false;
        });

        // Crash the test in case the timeout was reachaed without the callback being called
        connect(timeout, &QTimer::timeout, this, [&] {
#if defined(Q_CC_CLANG) && defined(Q_OS_WINDOWS)
            running = false;
            QEXPECT_FAIL("", "QTBUG-91768", Abort);
            QVERIFY(false);
            return;
#else
            QFAIL("Timeout waiting for QML continuation to be called");
#endif
        });
        while (running) {
            QCoreApplication::processEvents();
        }
    }
};

QTEST_GUILESS_MAIN(QCoroQmlTaskTest)

#include "qcoroqmltask.moc"
