#include <testhttpserver.hpp>
#include <testobject.hpp>
#include "qcoroiodevice_macros.hpp"
#include <testloop.hpp>
#include "testmacros.hpp"

#include <XQtHelper/qcoro/network/qcorolocalsocket.hpp>

#include <QLocalServer>
#include <QLocalSocket>

#include <thread>

static const QByteArray blockRequest { "GET /block HTTP/1.1\r\n" };
static const QByteArray streamRequest { "GET /stream HTTP/1.1\r\n" };

using namespace std::chrono_literals;

struct QCoroLocalSocketTest : QCoro::TestObject<QCoroLocalSocketTest> {
    Q_OBJECT

    TestHttpServer<QLocalServer> m_server_{};

    XUtils::XCoroTask<> testWaitForConnectedTriggers_coro(QCoro::TestContext) {
        QLocalSocket socket{};
        QCORO_DELAY(socket.connectToServer(QCoroLocalSocketTest::getSocketName()));

        co_await XUtils::qCoro(socket).waitForConnected();

        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);
        QCORO_VERIFY(m_server_.waitForConnection());
    }

    void testThenWaitForConnectedTriggers_coro(TestLoop &el) {
        QLocalSocket socket{};
        QCORO_DELAY(socket.connectToServer(QCoroLocalSocketTest::getSocketName()));
        bool called {};
        XUtils::qCoro(socket).waitForConnected().then([&](bool const connected) {
            called = true;
            el.quit();
            QVERIFY(connected);
        });
        el.exec();
        QVERIFY(called);
        QVERIFY(m_server_.waitForConnection());
    }

    XUtils::XCoroTask<> testWaitForDisconnectedTriggers_coro(QCoro::TestContext) {
        QLocalSocket socket{};
        socket.connectToServer(getSocketName());
        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);

        QCORO_DELAY(socket.disconnectFromServer());

        co_await XUtils::qCoro(socket).waitForDisconnected();

        QCORO_COMPARE(socket.state(), QLocalSocket::UnconnectedState);
        QCORO_VERIFY(m_server_.waitForConnection());
    }

    void testThenWaitForDisconnectedTriggers_coro(TestLoop &el) {
        QLocalSocket socket{};
        socket.connectToServer(getSocketName());
        QCOMPARE(socket.state(), QLocalSocket::ConnectedState);

        QCORO_DELAY(socket.disconnectFromServer());
        bool called {};
        XUtils::qCoro(socket).waitForDisconnected().then([&](bool const disconnected) {
            called = true;
            el.quit();
            QVERIFY(disconnected);
        });
        el.exec();
        QVERIFY(called);
        QVERIFY(m_server_.waitForConnection());
    }

    // On Linux at least, QLocalSocket connects immediately and synchronously
    XUtils::XCoroTask<> testDoesntCoAwaitConnectedSocket_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();

        QLocalSocket socket{};
        socket.connectToServer(getSocketName());

        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);

        co_await XUtils::qCoro(socket).waitForConnected();
        QCORO_VERIFY(m_server_.waitForConnection());
    }

    void testThenDoesntCoAwaitConnectedSocket_coro(TestLoop &el) {
        QLocalSocket socket;
        socket.connectToServer(getSocketName());
        QCOMPARE(socket.state(), QLocalSocket::ConnectedState);

        bool called {};
        XUtils::qCoro(socket).waitForConnected().then([&](bool const connected) {
            called = true;
            el.quit();
            QVERIFY(connected);
        });
        el.exec();
        QVERIFY(called);
        QVERIFY(m_server_.waitForConnection());
    }

    XUtils::XCoroTask<> testDoesntCoAwaitDisconnectedSocket_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();
        m_server_.setExpectTimeout(true);

        QLocalSocket socket{};
        QCORO_COMPARE(socket.state(), QLocalSocket::UnconnectedState);

        co_await XUtils::qCoro(socket).waitForDisconnected();
    }

    void testThenDoesntCoAwaitDisconnectedSocket_coro(TestLoop &el) {
        m_server_.setExpectTimeout(true);

        QLocalSocket socket;
        QCOMPARE(socket.state(), QLocalSocket::UnconnectedState);
        bool called {};
        XUtils::qCoro(socket).waitForDisconnected().then([&](bool const disconnected) {
            called = true;
            el.quit();
            QVERIFY(!disconnected);
        });
        el.exec();
        QVERIFY(called);
    }

    XUtils::XCoroTask<> testConnectToServerWithArgs_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();

        QLocalSocket socket {};

        co_await XUtils::qCoro(socket).connectToServer(getSocketName());

        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);
        QCORO_VERIFY(m_server_.waitForConnection());
    }

    void testThenConnectToServerWithArgs_coro(TestLoop &el) {
        QLocalSocket socket{};
        bool called {};
        XUtils::qCoro(socket).connectToServer(getSocketName()).then([&](bool const connected) {
            called = true;
            el.quit();
            QVERIFY(connected);
        });
        el.exec();
        QVERIFY(called);
        QVERIFY(m_server_.waitForConnection());
    }

    XUtils::XCoroTask<> testConnectToServer_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();

        QLocalSocket socket{};
        socket.setServerName(getSocketName());

        co_await XUtils::qCoro(socket).connectToServer();

        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);
        QCORO_VERIFY(m_server_.waitForConnection());
    }

    void testThenConnectToServer_coro(TestLoop &el) {
        QLocalSocket socket{};
        socket.setServerName(getSocketName());
        bool called {};
        XUtils::qCoro(socket).connectToServer().then([&](bool connected) {
            called = true;
            el.quit();
            QVERIFY(connected);
        });
        el.exec();
        QVERIFY(called);
        QVERIFY(m_server_.waitForConnection());
    }

    XUtils::XCoroTask<> testWaitForConnectedTimeout_coro(QCoro::TestContext) {
        m_server_.setExpectTimeout(true);
        QLocalSocket socket{};
        QCORO_TEST_TIMEOUT(co_await XUtils::qCoro(socket).waitForConnected(10ms));
    }

    void testThenWaitForConnectedTimeout_coro(TestLoop &el) {
        m_server_.setExpectTimeout(true);

        QLocalSocket socket{};

        bool called {};
        XUtils::qCoro(socket).waitForConnected(10ms).then([&](bool connected) {
            called = true;
            el.quit();
            QVERIFY(!connected);
        });
        auto const start{ std::chrono::steady_clock::now()};
        el.exec();
        auto const end{ std::chrono::steady_clock::now()};
        QVERIFY(end - start < 500ms);
        QVERIFY(called);
    }

    XUtils::XCoroTask<> testWaitForDisconnectedTimeout_coro(QCoro::TestContext) {
        QLocalSocket socket{};
        socket.connectToServer(getSocketName());
        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);

        QCORO_TEST_TIMEOUT(co_await XUtils::qCoro(socket).waitForDisconnected(10ms));
        QCORO_VERIFY(m_server_.waitForConnection());
    }

    void testThenWaitForDisconnectedTimeout_coro(TestLoop &el) {
        QLocalSocket socket{};
        socket.connectToServer(getSocketName());
        QCOMPARE(socket.state(), QLocalSocket::ConnectedState);
        bool called {};
        XUtils::qCoro(socket).waitForDisconnected(10ms).then([&](bool const disconnected) {
            called = true;
            el.quit();
            QVERIFY(!disconnected);
        });
        auto const start{ std::chrono::steady_clock::now()};
        el.exec();
        auto const end{ std::chrono::steady_clock::now()};
        QVERIFY(end - start < 500ms);
        QVERIFY(called);
        QVERIFY(m_server_.waitForConnection());
    }

    XUtils::XCoroTask<> testReadAllTriggers_coro(QCoro::TestContext) {
        QLocalSocket socket{};
        socket.connectToServer(getSocketName());
        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);
        auto const written{ co_await XUtils::qCoro(socket).write(streamRequest)};
        QCORO_COMPARE(written, streamRequest.size());
        using namespace XUtils;
        QCORO_TEST_IODEVICE_READALL(socket);
        QCORO_VERIFY(m_server_.waitForConnection());
    }

    void testThenReadAllTriggers_coro(TestLoop &el) {
        QLocalSocket socket{};
        socket.connectToServer(getSocketName());
        QCOMPARE(socket.state(), QLocalSocket::ConnectedState);
        bool called {};
        XUtils::qCoro(socket).readAll().then([&](QByteArray const & data) {
            called = true;
            el.quit();
            QVERIFY(!data.isEmpty());
        });

        XUtils::qCoro(socket).write(blockRequest).then([&](qint64 written) {
            QCOMPARE(written, blockRequest.size());
        });
        el.exec();

        QVERIFY(called);
        QVERIFY(m_server_.waitForConnection());
    }

    XUtils::XCoroTask<> testReadTriggers_coro(QCoro::TestContext) {
        QLocalSocket socket{};
        socket.connectToServer(getSocketName());
        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);

        auto const written{ co_await XUtils::qCoro(socket).write(streamRequest)};
        QCORO_COMPARE(written, streamRequest.size());
        using namespace XUtils;
        QCORO_TEST_IODEVICE_READ(socket);
        QCORO_VERIFY(m_server_.waitForConnection());
    }

    void testThenReadTriggers_coro(TestLoop &el) {
        QLocalSocket socket {};
        socket.connectToServer(getSocketName());
        QCOMPARE(socket.state(), QLocalSocket::ConnectedState);
        bool called {};
        XUtils::qCoro(socket).read(1).then([&](const QByteArray &data) {
            called = true;
            el.quit();
            QCOMPARE(data.size(), 1);
        });

        XUtils::qCoro(socket).write(blockRequest).then([&](qint64 written) {
            QCOMPARE(written, blockRequest.size());
        });

        el.exec();
        QVERIFY(called);
        QVERIFY(m_server_.waitForConnection());
    }

    XUtils::XCoroTask<> testReadLineTriggers_coro(QCoro::TestContext) {
        QLocalSocket socket{};
        socket.connectToServer(QCoroLocalSocketTest::getSocketName());
        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);

        auto const written{ co_await XUtils::qCoro(socket).write(streamRequest)};
        QCORO_COMPARE(written, streamRequest.size());
        using namespace XUtils;
        QCORO_TEST_IODEVICE_READLINE(socket);
        QCORO_COMPARE(lines.size(), 14);
        QCORO_VERIFY(m_server_.waitForConnection());
    }

    void testThenReadLineTriggers_coro(TestLoop &el) {
        QLocalSocket socket{};
        socket.connectToServer(getSocketName());
        QCOMPARE(socket.state(), QLocalSocket::ConnectedState);
        bool called {};
        XUtils::qCoro(socket).readLine().then(
        [&](QByteArray const & data){ called = true;el.quit();QVERIFY(!data.isEmpty()); }
        );

        XUtils::qCoro(socket).write(blockRequest).then([&](qint64 const written)
        { QCOMPARE(written, blockRequest.size()); });

        el.exec();

        QVERIFY(called);
        QVERIFY(m_server_.waitForConnection());
    }

private Q_SLOTS:
    void init() { m_server_.start(getSocketName()); }
    void cleanup() { m_server_.stop(); }

    addCoroAndThenTests(WaitForConnectedTriggers)
    addCoroAndThenTests(WaitForConnectedTimeout)
    addCoroAndThenTests(WaitForDisconnectedTriggers)
    addCoroAndThenTests(WaitForDisconnectedTimeout)
    addCoroAndThenTests(DoesntCoAwaitConnectedSocket)
    addCoroAndThenTests(DoesntCoAwaitDisconnectedSocket)
    addCoroAndThenTests(ConnectToServerWithArgs)
    addCoroAndThenTests(ConnectToServer)
    addCoroAndThenTests(ReadAllTriggers)
    addCoroAndThenTests(ReadTriggers)
    addCoroAndThenTests(ReadLineTriggers)

private:
    static QString getSocketName() {
        return QStringLiteral("%1-%2")
            .arg(QCoreApplication::applicationName())
            .arg(QCoreApplication::applicationPid());
    }
};

QTEST_GUILESS_MAIN(QCoroLocalSocketTest)

#include "qcorolocalsocket.moc"
