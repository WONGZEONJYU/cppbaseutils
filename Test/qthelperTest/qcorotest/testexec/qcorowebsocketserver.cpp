#include <XQtHelper/qcoro/websockets/qcorowebsocketserver.hpp>
#include <XQtHelper/qcoro/websockets/qcorowebsocket.hpp>
#include <XQtHelper/qcoro/core/waitfor.hpp>

#include <testloop.hpp>
#include <testobject.hpp>
#include <testmacros.hpp>

#include <QWebSocketServer>
#include <QWebSocket>

#include <QTest>

using namespace std::chrono_literals;

struct QCoroWebSocketServerTest : QCoro::TestObject<QCoroWebSocketServerTest> {
    Q_OBJECT

public:
    explicit(false) QCoroWebSocketServerTest(QObject * const parent = {})
        : TestObject{ parent } {
        // On Windows, constructing QWebSocket for the first time takes some time
        // (most likely due to loading OpenSSL), which causes the first test to
        // time out on the CI.
        QWebSocket socket{};
    }

private:
    XUtils::XCoroTask<> testNextPendingConnection_coro(QCoro::TestContext) {
        QWebSocketServer server{ QStringLiteral("TestWSServer"), QWebSocketServer::NonSecureMode };
        QCORO_VERIFY(server.listen(QHostAddress::LocalHost));

        QWebSocket socket;
        QCORO_DELAY(socket.open(server.serverUrl()));
        const auto serverSocket = std::unique_ptr<QWebSocket>(co_await XUtils::qCoro(server).nextPendingConnection());
        QCORO_VERIFY(serverSocket != nullptr);
    }

    void testThenNextPendingConnection_coro(TestLoop &el) {
        QWebSocketServer server {QStringLiteral("TestWSServer"), QWebSocketServer::NonSecureMode};
        QVERIFY(server.listen(QHostAddress::LocalHost));

        QWebSocket socket{};
        QCORO_DELAY(socket.open(server.serverUrl()));

        bool called = false;
        XUtils::qCoro(server).nextPendingConnection().then([&el, &called](QWebSocket * const socket_) {
            el.quit();
            called = true;
            QVERIFY(socket_ != nullptr);
            socket_->deleteLater();
        });
        el.exec();
        QVERIFY(called);
    }

    XUtils::XCoroTask<> testNextPendingConnectionTimeout_coro(QCoro::TestContext) {
        QWebSocketServer server {QStringLiteral("TestWSServer"), QWebSocketServer::NonSecureMode};
        QCORO_VERIFY(server.listen(QHostAddress::LocalHost));
        auto const socket { co_await XUtils::qCoro(server).nextPendingConnection(10ms) };
        QCORO_COMPARE(socket, nullptr);
    }

    static void testThenNextPendingConnectionTimeout_coro(TestLoop & el) {
        QWebSocketServer server {QStringLiteral("TestWSServer"), QWebSocketServer::NonSecureMode};
        QVERIFY(server.listen(QHostAddress::LocalHost));

        bool called {};
        XUtils::qCoro(server).nextPendingConnection(100ms).then([&el, &called](QWebSocket * const socket) {
            el.quit();
            called = true;
            QCOMPARE(socket, nullptr);
        });
        el.exec();
        QVERIFY(called);
    }

    XUtils::XCoroTask<> testClosingServerResumesAwaiters_coro(QCoro::TestContext) {
        QWebSocketServer server {QStringLiteral("TestWSServer"), QWebSocketServer::NonSecureMode};
        QCORO_VERIFY(server.listen(QHostAddress::LocalHost));
        QCORO_DELAY(server.close());
        auto const socket { co_await XUtils::qCoro(server).nextPendingConnection() };
        QCORO_COMPARE(socket, nullptr);
    }

    static void testThenClosingServerResumesAwaiters_coro(TestLoop & el) {
        QWebSocketServer server { QStringLiteral("TestWSServer"), QWebSocketServer::NonSecureMode };
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QCORO_DELAY(server.close());

        bool called {};
        XUtils::qCoro(server).nextPendingConnection().then([&el, &called](QWebSocket * const socket) {
            el.quit();
            called = true;
            QCOMPARE(socket, nullptr);
        });
        el.exec();
        QVERIFY(called);
    }

    XUtils::XCoroTask<> testDoesntCoawaitNonlisteningServer_coro(QCoro::TestContext ctx) {
        ctx.setShouldNotSuspend();
        QWebSocketServer server { QStringLiteral("TestWSServer"), QWebSocketServer::NonSecureMode };
        auto const socket { co_await XUtils::qCoro(server).nextPendingConnection() };
        QCORO_COMPARE(socket, nullptr);
    }

    XUtils::XCoroTask<> testDoesntCoawaitWithPendingConnection_coro(QCoro::TestContext ctx) {
        ctx.setShouldNotSuspend();

        QWebSocketServer server { QStringLiteral("TestWSServer"), QWebSocketServer::NonSecureMode };
        QCORO_VERIFY(server.listen(QHostAddress::LocalHost));

        QWebSocket socket{};
        QCORO_VERIFY(XUtils::waitFor(XUtils::qCoro(socket).open(server.serverUrl())));
        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);

        QTest::qWait(100); // give the server time to register the incoming connection
        QCORO_VERIFY(server.hasPendingConnections());

        const auto serverSocket = std::unique_ptr<QWebSocket>(co_await XUtils::qCoro(server).nextPendingConnection());
        QCORO_VERIFY(serverSocket);
    }

private Q_SLOTS:
    addCoroAndThenTests(NextPendingConnection)
    addCoroAndThenTests(NextPendingConnectionTimeout)
    addCoroAndThenTests(ClosingServerResumesAwaiters)
    addTest(DoesntCoawaitNonlisteningServer)
    addTest(DoesntCoawaitWithPendingConnection)
};

QTEST_GUILESS_MAIN(QCoroWebSocketServerTest)

#include "qcorowebsocketserver.moc"
