#include <testdbusserver.hpp>
#include <testobject.hpp>
#include <XQtHelper/qcoro/dbus/qcorodbuspendingcall.hpp>
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusInterface>
#include <QDBusReply>

struct QCoroDBusPendingCallTest : QCoro::TestObject<QCoroDBusPendingCallTest> {
    Q_OBJECT

    XUtils::XCoroTaskVoid testTriggers_coro(QCoro::TestContext) {
        QDBusInterface iface { DBusServer::serviceName, DBusServer::objectPath,DBusServer::interfaceName };
        QCORO_VERIFY(iface.isValid());

        QDBusReply<void> const reply {  co_await iface.asyncCall(QStringLiteral("foo")) };
        QCORO_VERIFY(reply.isValid());
    }

    XUtils::XCoroTaskVoid testReturnsResult_coro(QCoro::TestContext) {
        QDBusInterface iface {DBusServer::serviceName, DBusServer::objectPath,DBusServer::interfaceName };
        QCORO_VERIFY(iface.isValid());

        QDBusReply<QString> const reply { co_await iface.asyncCall(QStringLiteral("ping"), QStringLiteral("Hello there!")) };

        QCORO_VERIFY(reply.isValid());
        QCORO_COMPARE(reply.value(), QStringLiteral("Hello there!"));
    }

    void testThenReturnsResult_coro(TestLoop &el) {

        QDBusInterface iface {DBusServer::serviceName, DBusServer::objectPath,DBusServer::interfaceName };
        QVERIFY(iface.isValid());

        QDBusPendingCall const call { iface.asyncCall(QStringLiteral("ping"), QStringLiteral("Hello there!")) };

        bool called {};
        XUtils::qCoro(call).waitForFinished().then([&](QDBusMessage const &msg) {
            called = true;
            el.quit();
            QCOMPARE(QDBusReply<QString>(msg).value(), QStringLiteral("Hello there!"));
        });
        el.exec();
        QVERIFY(called);
    }

    XUtils::XCoroTaskVoid testDoesntBlockEventLoop_coro(QCoro::TestContext) {
        QCoro::EventLoopChecker const eventLoopResponsive {};
        QDBusInterface iface{DBusServer::serviceName, DBusServer::objectPath,DBusServer::interfaceName};
        QCORO_VERIFY(iface.isValid());

        QDBusReply<void> const reply { co_await iface.asyncCall(QStringLiteral("blockFor"), 1) };

        QCORO_VERIFY(reply.isValid());
        QCORO_VERIFY(eventLoopResponsive);
    }

    XUtils::XCoroTaskVoid testDoesntCoAwaitFinishedCall_coro(QCoro::TestContext test) {
        QDBusInterface iface{DBusServer::serviceName, DBusServer::objectPath,DBusServer::interfaceName};
        QCORO_VERIFY(iface.isValid());

        auto call { iface.asyncCall(QStringLiteral("foo")) };
        QDBusReply<void> reply { co_await call};
        QCORO_VERIFY(reply.isValid());
        test.setShouldNotSuspend();
        reply = co_await call;
        QCORO_VERIFY(reply.isValid());
    }

private Q_SLOTS:
    void initTestCase() {
        for (int i {}; i < 10; ++i) {
            if (QDBusInterface iface{DBusServer::serviceName, DBusServer::objectPath,DBusServer::interfaceName};
                iface.isValid())
            { return; }
            QTest::qWait(100);
        }

        QFAIL("Failed to obtain a valid dbus interface");
    }

    void cleanupTestCase() {
        QDBusInterface iface{DBusServer::serviceName, DBusServer::objectPath,DBusServer::interfaceName};
        iface.call(QStringLiteral("quit"));
    }

    addTest(Triggers)
    addCoroAndThenTests(ReturnsResult)
    addTest(DoesntBlockEventLoop)
    addTest(DoesntCoAwaitFinishedCall)
};

DBUS_TEST_MAIN(QCoroDBusPendingCallTest)

#include "qdbuspendingcall.moc"
