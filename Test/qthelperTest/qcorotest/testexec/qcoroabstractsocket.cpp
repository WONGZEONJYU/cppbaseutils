#include <testhttpserver.hpp>
#include <testobject.hpp>
#include "qcoroiodevice_macros.hpp"
#include <XQtHelper/qcoro/network/qcoroabstractsocket.hpp>
#include <testmacros.hpp>

#include <QTcpServer>
#include <QTcpSocket>

#include <thread>

using namespace std::chrono_literals;

struct QCoroAbstractSocketTest : QCoro::TestObject<QCoroAbstractSocketTest> {
    Q_OBJECT
    TestHttpServer<QTcpServer> m_server_{};

    XUtils::XCoroTask<> testWaitForConnectedTriggers_coro(QCoro::TestContext) {
        QTcpSocket socket{};
        QCORO_DELAY(socket.connectToHost(QHostAddress::LocalHost, m_server_.port()));

        co_await XUtils::qCoro(socket).waitForConnected();

        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);

        // Make sure the server gets the connection as well
        QCORO_VERIFY(m_server_.waitForConnection());
    }

    void testThenWaitForConnectedTriggers_coro(TestLoop &el) {
        QTcpSocket socket{};
        QCORO_DELAY(socket.connectToHost(QHostAddress::LocalHost, m_server_.port()));
        bool called {};
        XUtils::qCoro(socket).waitForConnected().then([&](bool connected) {
            called = true;
            el.quit();
            QVERIFY(connected);
        });
        el.exec();
        QVERIFY(called);
        QVERIFY(m_server_.waitForConnection());
    }

    XUtils::XCoroTask<> testWaitForDisconnectedTriggers_coro(QCoro::TestContext) {
        QTcpSocket socket{};
        co_await XUtils::qCoro(socket).connectToHost(QHostAddress::LocalHost, m_server_.port());
        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);

        QCORO_DELAY(socket.disconnectFromHost());

        co_await XUtils::qCoro(socket).waitForDisconnected();

        QCORO_COMPARE(socket.state(), QAbstractSocket::UnconnectedState);

        QCORO_VERIFY(m_server_.waitForConnection());
    }

    void testThenWaitForDisconnectedTriggers_coro(TestLoop &el) {
        QTcpSocket socket{};
        bool called {};

        XUtils::qCoro(socket).connectToHost(QHostAddress::LocalHost, m_server_.port()).then([&](bool const connected) {
            if (!connected) {
                el.quit();
            }
            QVERIFY(connected);
            QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);

            QCORO_DELAY(socket.disconnectFromHost());

            XUtils::qCoro(socket).waitForDisconnected().then([&](bool const connected_) {
                called = true;
                el.quit();
                QVERIFY(connected_);
            });
        });
        el.exec();

        QVERIFY(called);

        QVERIFY(m_server_.waitForConnection());
    }

    XUtils::XCoroTask<> testDoesntCoAwaitConnectedSocket_coro(QCoro::TestContext context) {
        QTcpSocket socket{};
        co_await XUtils::qCoro(socket).connectToHost(QHostAddress::LocalHost, m_server_.port());

        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);

        context.setShouldNotSuspend();
        co_await XUtils::qCoro(socket).waitForConnected();

        socket.write("GET / HTTP/1.1\r\n");

        QCORO_VERIFY(m_server_.waitForConnection());
    }

    void testThenDoesntCoAwaitConnectedSocket_coro(TestLoop &el) {
        QTcpSocket socket{};
        bool called {};
        XUtils::qCoro(socket).connectToHost(QHostAddress::LocalHost, m_server_.port()).then([&](bool const connected) {
            if (!connected) {
                el.quit();
            }
            QVERIFY(connected);
            QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);

            XUtils::qCoro(socket).waitForConnected().then([&](bool const connected_) {
                called = true;
                el.quit();
                QVERIFY(connected_);
            });
        });
        el.exec();

        QVERIFY(called);

        QVERIFY(m_server_.waitForConnection());
    }

    XUtils::XCoroTask<> testDoesntCoAwaitDisconnectedSocket_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();
        m_server_.setExpectTimeout(true); // no-one actually connects, so the server times out.

        QTcpSocket socket {};
        QCORO_COMPARE(socket.state(), QAbstractSocket::UnconnectedState);

        co_await XUtils::qCoro(socket).waitForDisconnected();
    }

    void testThenDoesntCoAwaitDisconnectedSocket_coro(TestLoop &el) {
        m_server_.setExpectTimeout(true); // no-one actually connects, so the server time out.

        QTcpSocket socket{};
        QCOMPARE(socket.state(), QAbstractSocket::UnconnectedState);

        bool called {};
        XUtils::qCoro(socket).waitForDisconnected().then([&](bool const disconnected) {
            called = true;
            el.quit();
            QVERIFY(!disconnected);
        });
        el.exec();

        QVERIFY(called);
    }

    XUtils::XCoroTask<> testConnectToServerWithArgs_coro(QCoro::TestContext) {
        QTcpSocket socket {};

        co_await XUtils::qCoro(socket).connectToHost(QHostAddress::LocalHost, m_server_.port());

        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);
        QCORO_VERIFY(m_server_.waitForConnection());
    }

    void testThenConnectToServerWithArgs_coro(TestLoop &el) {
        QTcpSocket socket{};
        bool called {};
        XUtils::qCoro(socket).connectToHost(QHostAddress::LocalHost, m_server_.port()).then([&](bool const connected) {
            called = true;
            el.quit();
            QVERIFY(connected);
            QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
        });
        el.exec();
        QVERIFY(called);
        QVERIFY(m_server_.waitForConnection());
    }

    XUtils::XCoroTask<> testWaitForConnectedTimeout_coro(QCoro::TestContext) {
        m_server_.setExpectTimeout(true);
        QTcpSocket socket{};
        QCORO_TEST_TIMEOUT(co_await XUtils::qCoro(socket).waitForConnected(10ms));
    }

    void testThenWaitForConnectedTimeout_coro(TestLoop &el) {
        m_server_.setExpectTimeout(true);

        QTcpSocket socket{};
        auto const start{ std::chrono::steady_clock::now()};
        bool called {};
        XUtils::qCoro(socket).waitForConnected(10ms).then([&](bool const connected) {
            called = true;
            el.quit();
            QVERIFY(!connected);
        });
        el.exec();
        auto const end{ std::chrono::steady_clock::now()};
        QVERIFY(end - start < 500ms);
        QVERIFY(called);
    }

    XUtils::XCoroTask<> testWaitForDisconnectedTimeout_coro(QCoro::TestContext) {
        m_server_.setExpectTimeout(true);

        QTcpSocket socket{};
        co_await XUtils::qCoro(socket).connectToHost(QHostAddress::LocalHost, m_server_.port());
        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);

        QCORO_TEST_TIMEOUT(co_await XUtils::qCoro(socket).waitForDisconnected(10ms));

        QCORO_VERIFY(m_server_.waitForConnection());
    }

    void testThenWaitForDisconnectedTimeout_coro(TestLoop &el) {
        m_server_.setExpectTimeout(true);

        QTcpSocket socket{};
        XUtils::qCoro(socket).connectToHost(QHostAddress::LocalHost, m_server_.port()).then([&](bool const connected) {
            if (!connected) {
                el.quit();
            }
            QVERIFY(connected);
            QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);

            auto const start{ std::chrono::steady_clock::now()};
            XUtils::qCoro(socket).waitForDisconnected(10ms).then([&el, start](bool const disconnected) {
                el.quit();
                QVERIFY(!disconnected);
                auto const end{ std::chrono::steady_clock::now()};
                QVERIFY(end - start < 500ms);
            });
        });
        el.exec();
        QVERIFY(m_server_.waitForConnection());
    }

    XUtils::XCoroTask<> testReadAllTriggers_coro(QCoro::TestContext) {
        QTcpSocket socket{};
        co_await XUtils::qCoro(socket).connectToHost(QHostAddress::LocalHost, m_server_.port());
        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);

        socket.write("GET /stream HTTP/1.1\r\n");
        using namespace XUtils;
        QCORO_TEST_IODEVICE_READALL(socket);
        QCORO_VERIFY(m_server_.waitForConnection());
    }

    void testThenReadAllTriggers_coro(TestLoop &el) {
        QTcpSocket socket{};
        bool called {};
        XUtils::qCoro(socket).connectToHost(QHostAddress::LocalHost, m_server_.port()).then([&](bool const connected) {
            if (!connected) {
                el.quit();
            }
            QVERIFY(connected);
            QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);

            socket.write("GET /block HTTP/1.1\r\n");

            QByteArray data{};
            XUtils::qCoro(socket).readAll().then([&](QByteArray const & d) {
                el.quit();
                called = true;
                QVERIFY(!d.isEmpty());
            });
        });
        el.exec();

        QVERIFY(called);
        QVERIFY(m_server_.waitForConnection());
    }

    XUtils::XCoroTask<> testReadTriggers_coro(QCoro::TestContext) {
        QTcpSocket socket{};
        co_await XUtils::qCoro(socket).connectToHost(QHostAddress::LocalHost, m_server_.port());
        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);

        socket.write("GET /stream HTTP/1.1\r\n");
        using namespace XUtils;
        QCORO_TEST_IODEVICE_READ(socket);
        QCORO_VERIFY(m_server_.waitForConnection());
    }

     void testThenReadTriggers_coro(TestLoop &el) {
        QTcpSocket socket{};
        bool called {};
        XUtils::qCoro(socket).connectToHost(QHostAddress::LocalHost, m_server_.port()).then([&](bool const connected) {
            if (!connected) {
                el.quit();
            }
            QVERIFY(connected);
            QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);

            socket.write("GET /block HTTP/1.1\r\n");

            QByteArray data{};
            XUtils::qCoro(socket).read(1).then([&](QByteArray const & d) {
                el.quit();
                called = true;
                QCOMPARE(d.size(), 1);
            });
        });
        el.exec();

        QVERIFY(called);
        QVERIFY(m_server_.waitForConnection());
    }

    XUtils::XCoroTask<> testReadLineTriggers_coro(QCoro::TestContext) {
        QTcpSocket socket{};
        co_await XUtils::qCoro(socket).connectToHost(QHostAddress::LocalHost, m_server_.port());
        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);

        socket.write("GET /stream HTTP/1.1\r\n");
        using namespace XUtils;
        QCORO_TEST_IODEVICE_READLINE(socket);
        QCORO_COMPARE(lines.size(), 14);
        QCORO_VERIFY(m_server_.waitForConnection());
    }

     void testThenReadLineTriggers_coro(TestLoop &el) {
        QTcpSocket socket{};
        bool called {};
        XUtils::qCoro(socket).connectToHost(QHostAddress::LocalHost, m_server_.port()).then([&](bool const connected) {
            if (!connected) {
                el.quit();
            }
            QVERIFY(connected);
            QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);

            socket.write("GET /stream HTTP/1.1\r\n");

            QByteArray data;
            XUtils::qCoro(socket).readLine().then([&](QByteArray const & d) {
                el.quit();
                called = true;
                QVERIFY(!d.isEmpty());
            });
        });
        el.exec();

        QVERIFY(called);
        QVERIFY(m_server_.waitForConnection());
    }

private Q_SLOTS:
    void init() { m_server_.start(QHostAddress::LocalHost); }

    void cleanup() { m_server_.stop(); }

    addCoroAndThenTests(WaitForConnectedTriggers)
    addCoroAndThenTests(WaitForConnectedTimeout)
    addCoroAndThenTests(WaitForDisconnectedTriggers)
    addCoroAndThenTests(WaitForDisconnectedTimeout)
    addCoroAndThenTests(DoesntCoAwaitConnectedSocket)
    addCoroAndThenTests(DoesntCoAwaitDisconnectedSocket)
    addCoroAndThenTests(ConnectToServerWithArgs)
    addCoroAndThenTests(ReadAllTriggers)
    addCoroAndThenTests(ReadTriggers)
    addCoroAndThenTests(ReadLineTriggers)
};

QTEST_GUILESS_MAIN(QCoroAbstractSocketTest)

#include "qcoroabstractsocket.moc"
