#include <testobject.hpp>
#include <XQtHelper/qcoro/network/qcorotcpserver.hpp>
#include <XQtHelper/qcoro/network/qcoroabstractsocket.hpp>
#include <QTcpServer>
#include <QTcpSocket>
#include <thread>
#include <mutex>

using namespace std::chrono_literals;

class Client {
    std::thread m_thread_{};
public:
    Client(uint16_t serverPort, std::mutex & mutex, bool & ok)
        : m_thread_([serverPort, &mutex, &ok] {
            std::this_thread::sleep_for(500ms);

            std::unique_lock lock{mutex};
            QTcpSocket socket{};
            socket.connectToHost(QHostAddress::LocalHost, serverPort);
            if (!socket.waitForConnected(10'000)) {
                qWarning() << "Not connected within timeout" << socket.errorString();
                ok = false;
                return;
            }
            socket.write("Hello World!");
            socket.flush();
            socket.close();
            ok = true;
        })
    {}

    ~Client() { m_thread_.join(); }
};

class QCoroTcpServerTest: public QCoro::TestObject<QCoroTcpServerTest> {
    Q_OBJECT

    XUtils::XCoroTask<> testWaitForNewConnectionTriggers_coro(QCoro::TestContext) {
        QTcpServer server{};
        QCORO_VERIFY(server.listen(QHostAddress::LocalHost));
        QCORO_VERIFY(server.isListening());
        auto const serverPort { server.serverPort()};

        std::mutex mutex{};
        bool ok {};
        Client client(serverPort, mutex, ok);

        auto const connection{ co_await XUtils::qCoro(server).waitForNewConnection(10s)};
        QCORO_VERIFY(connection != nullptr);
        auto const data{ co_await XUtils::qCoro(connection).readAll()};
        QCORO_COMPARE(data, QByteArray{"Hello World!"});

        std::unique_lock lock{mutex};
        QCORO_VERIFY(ok);
    }

    static void testThenWaitForNewConnectionTriggers_coro(TestLoop &el) {
        QTcpServer server{};
        QVERIFY(server.listen(QHostAddress::LocalHost));
        const quint16 serverPort = server.serverPort();

        std::mutex mutex{};
        bool ok {};
        Client client(serverPort, mutex, ok);

        bool called {};
        XUtils::qCoro(server).waitForNewConnection(10s).then([&](QTcpSocket *socket) -> XUtils::XCoroTask<> {
            called = true;
            if (!socket) {
                el.quit();
                co_return;
            }

            auto const data{ co_await XUtils::qCoro(socket).readAll()};
            QCORO_COMPARE(data, QByteArray("Hello World!"));
            el.quit();
        });
        el.exec();

        std::lock_guard lock{mutex};
        QVERIFY(called);
        QVERIFY(ok);
    }

    XUtils::XCoroTask<> testDoesntCoAwaitPendingConnection_coro(QCoro::TestContext testContext) {
        testContext.setShouldNotSuspend();

        QTcpServer server{};
        QCORO_VERIFY(server.listen(QHostAddress::LocalHost));
        auto const serverPort{ server.serverPort() };

        bool ok {};
        std::mutex mutex{};
        Client client(serverPort, mutex, ok);

        QCORO_VERIFY(server.waitForNewConnection(10'000));

        auto const connection{ co_await XUtils::qCoro(server).waitForNewConnection(10s)};

        connection->waitForReadyRead(); // can't use coroutine, it might suspend or not, depending on how eventloop
                                        // gets triggered, which fails the test since it's setShouldNotSuspend()
        QCORO_COMPARE(connection->readAll(), QByteArray{"Hello World!"});

        std::unique_lock lock{mutex};
        QCORO_VERIFY(ok);
    }

private Q_SLOTS:
    addCoroAndThenTests(WaitForNewConnectionTriggers)
    addTest(DoesntCoAwaitPendingConnection)
};

QTEST_GUILESS_MAIN(QCoroTcpServerTest)

#include "qcorotcpserver.moc"
