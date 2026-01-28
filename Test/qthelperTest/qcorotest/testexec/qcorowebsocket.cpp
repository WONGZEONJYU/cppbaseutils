#include <testobject.hpp>
#include <testwsserver.hpp>

#include <testmacros.hpp>

#include <XQtHelper/qcoro/websockets/qcorowebsocket.hpp>
#include <XQtHelper/qcoro/core/waitfor.hpp>

#include <QWebSocket>

class QCoroWebSocketTest : public QCoro::TestObject<QCoroWebSocketTest> {
    Q_OBJECT
    TestWsServer m_server_{};

public:
    Q_IMPLICIT QCoroWebSocketTest(QObject * const parent = {}): TestObject{parent} {
        // On Windows, constructing QWebSocket for the first time takes some time
        // (most likely due to loading OpenSSL), which causes the first test to
        // time out on the CI.
        QWebSocket socket {};
    }

private:
    template<typename T, typename SendFunc, typename RecvFunc>
    XUtils::XCoroTask<> testReceived(T && msg, SendFunc && sendFunc, RecvFunc && recvFunc) {
        QWebSocket socket {};

        QCORO_VERIFY(connectSocket(socket));
        using namespace std::chrono_literals;
        QCORO_DELAY(std::invoke(std::forward<SendFunc>(sendFunc), socket, std::forward<T>(msg)));

        auto const coroSocket { XUtils::qCoro(socket)};

        auto gen { std::invoke(std::forward<RecvFunc>(recvFunc), std::addressof(coroSocket), std::chrono::milliseconds{-1}) };
        auto const data { co_await gen.begin() };

        QCORO_VERIFY(data != gen.end());
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(*data)>, QString> ||
                      std::is_same_v<std::remove_cvref_t<decltype(*data)>, QByteArray>) {
            QCORO_COMPARE(*data, msg);
        } else {
            QCORO_COMPARE(std::get<0>(*data), msg);
            QCORO_COMPARE(std::get<1>(*data), true);
        }
    }

    template<typename RecvFunc>
    XUtils::XCoroTask<> testTimeout(RecvFunc && recvFunc) {
        m_server_.setExpectTimeout();

        QWebSocket socket{};
        QCORO_VERIFY(connectSocket(socket));

        auto const coroSocket { XUtils::qCoro(socket) };
        using namespace std::chrono_literals;
        auto gen { std::invoke(std::forward<RecvFunc>(recvFunc), std::addressof(coroSocket), 10ms)};
        auto const data { co_await gen.begin()};
        QCORO_COMPARE(data, gen.end());
    }

    template<typename RecvFunc>
    XUtils::XCoroTask<> testGeneratorEndOnSocketClose(RecvFunc && recvFunc) {
        m_server_.setExpectTimeout();

        QWebSocket socket {};
        QCORO_VERIFY(connectSocket(socket));
        using namespace std::chrono_literals;
        QCORO_DELAY(socket.close());
        auto const coroSocket { XUtils::qCoro(socket) };
        auto gen { std::invoke(recvFunc, std::addressof(coroSocket), std::chrono::milliseconds{-1}) };
        auto const it { co_await gen.begin()};
        QCORO_COMPARE(it, gen.end());
    }

    XUtils::XCoroTask<> testWaitForOpenWithUrl_coro(QCoro::TestContext) {
        QWebSocket socket{};
        auto const result{ co_await XUtils::qCoro(socket).open(m_server_.url()) };
        QCORO_VERIFY(result);
        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);
        QCORO_VERIFY(m_server_.waitForConnection());
    }

    void testThenWaitForOpenWithUrl_coro(TestLoop & el) const {
        QWebSocket socket{};
        bool called {};
        XUtils::qCoro(socket).open(m_server_.url()).then([&el, &called](bool const connected) {
            called = true;
            el.quit();
            QVERIFY(connected);
        });
        el.exec();
        QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
        QVERIFY(called);
        QVERIFY(m_server_.waitForConnection());
    }

    XUtils::XCoroTask<> testTimeoutOpenWithUrl_coro(QCoro::TestContext) {
        QWebSocket socket{};
        auto const url { m_server_.url() };
        m_server_.stop(); // stop the server so we cannot connect
        using namespace std::chrono_literals;
        const auto result = co_await XUtils::qCoro(socket).open(url, 10ms);
        QCORO_VERIFY(!result);
    }

    void testThenTimeoutOpenWithUrl_coro(TestLoop & el) {
        QWebSocket socket{};
        auto const url{ m_server_.url() };
        m_server_.stop();
        bool called {};
        using namespace std::chrono_literals;
        XUtils::qCoro(socket).open(url, 10ms).then([&el, &called](bool connected) {
            el.quit();
            called = true;
            QVERIFY(!connected);
        });
        el.exec();
        QVERIFY(called);
    }

    XUtils::XCoroTask<> testWaitForOpenWithNetworkRequest_coro(QCoro::TestContext) {
        QWebSocket socket{};
        QNetworkRequest const request { m_server_.url() };
        auto const result{ co_await XUtils::qCoro(socket).open(request) };
        QCORO_VERIFY(result);
        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);
        QCORO_VERIFY(m_server_.waitForConnection());
    }

    void testThenWaitForOpenWithNetworkRequest_coro(TestLoop &el) const {
        QWebSocket socket{};
        bool called {};
        XUtils::qCoro(socket).open(QNetworkRequest{m_server_.url()}).then([&el, &called](bool const connected) {
            called = true;
            el.quit();
            QVERIFY(connected);
        });
        el.exec();
        QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
        QVERIFY(called);
        QVERIFY(m_server_.waitForConnection());
    }

    XUtils::XCoroTask<> testDoesntCoawaitOpenedSocket_coro(QCoro::TestContext ctx) {
        QWebSocket socket{};
        QCORO_VERIFY(connectSocket(socket));

        ctx.setShouldNotSuspend();

        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);
        auto const connected{ co_await XUtils::qCoro(socket).open(m_server_.url()) };
        QCORO_VERIFY(connected);
    }

    XUtils::XCoroTask<> testPing_coro(QCoro::TestContext) {
        QWebSocket socket{};
        QCORO_VERIFY(connectSocket(socket));

        auto const response{ co_await XUtils::qCoro(socket).ping("PING!") };
        QCORO_VERIFY(response.has_value());
        using namespace std::chrono_literals;
        QCORO_VERIFY(*response >= 0ms); // the latency will be somewhere around 0
    }

    void testThenPing_coro(TestLoop &el) const {
        QWebSocket socket;
        QVERIFY(connectSocket(socket));
        bool called {};
        XUtils::qCoro(socket).ping("PING!").then([&el, &called](std::optional<std::chrono::milliseconds> const pong) {
            el.quit();
            called = true;
            QVERIFY(pong.has_value());
            using namespace std::chrono_literals;
            QVERIFY(*pong >= 0ms);
        });
        el.exec();
        QVERIFY(called);
    }

    XUtils::XCoroTask<> testBinaryFrame_coro(QCoro::TestContext) {
        co_await testReceived(QByteArray("TEST MESSAGE"), &QWebSocket::sendBinaryMessage,
                              &XUtils::detail::QCoroWebSocket::binaryFrames);
    }

    XUtils::XCoroTask<> testBinaryFrameTimeout_coro(QCoro::TestContext) {
        co_await testTimeout(&XUtils::detail::QCoroWebSocket::binaryFrames);
    }

    XUtils::XCoroTask<> testBinaryFrameGeneratorEndsOnSocketClose_coro(QCoro::TestContext) {
        co_await testGeneratorEndOnSocketClose(&XUtils::detail::QCoroWebSocket::binaryFrames);
    }

    XUtils::XCoroTask<> testBinaryMessage_coro(QCoro::TestContext) {
        co_await testReceived(QByteArray("TEST MESSAGE"), &QWebSocket::sendBinaryMessage,
                              &XUtils::detail::QCoroWebSocket::binaryMessages);
    }

    XUtils::XCoroTask<> testBinaryMessageTimeout_coro(QCoro::TestContext) {
        co_await testTimeout(&XUtils::detail::QCoroWebSocket::binaryMessages);
    }

    XUtils::XCoroTask<> testBinaryMessageGeneratorEndsOnSocketClose_coro(QCoro::TestContext) {
        co_await testGeneratorEndOnSocketClose(&XUtils::detail::QCoroWebSocket::binaryMessages);
    }

    XUtils::XCoroTask<> testTextFrame_coro(QCoro::TestContext) {
        co_await testReceived(QStringLiteral("TEST MESSAGE"), &QWebSocket::sendTextMessage,
                             &XUtils::detail::QCoroWebSocket::textFrames);
    }

    XUtils::XCoroTask<> testTextFrameTimeout_coro(QCoro::TestContext) {
        co_await testTimeout(&XUtils::detail::QCoroWebSocket::textFrames);
    }

    XUtils::XCoroTask<> testTextFrameGeneratorEndsOnSocketClose_coro(QCoro::TestContext) {
        co_await testGeneratorEndOnSocketClose(&XUtils::detail::QCoroWebSocket::textFrames);
    }

    XUtils::XCoroTask<> testTextMessage_coro(QCoro::TestContext) {
        co_await testReceived(QStringLiteral("TEST MESSAGE"), &QWebSocket::sendTextMessage,
                              &XUtils::detail::QCoroWebSocket::textMessages);
    }

    XUtils::XCoroTask<> testTextMessageTimeout_coro(QCoro::TestContext) {
        co_await testTimeout(&XUtils::detail::QCoroWebSocket::textMessages);
    }

    XUtils::XCoroTask<> testTextMessageGeneratorEndsOnSocketClose_coro(QCoro::TestContext) {
        co_await testGeneratorEndOnSocketClose(&XUtils::detail::QCoroWebSocket::textMessages);
    }

    XUtils::XCoroTask<> testReadFragmentedMessage_coro(QCoro::TestContext) {
        QWebSocket socket;
        QUrl url = m_server_.url();
        url.setPath(QStringLiteral("/large"));
        QCORO_VERIFY(XUtils::waitFor(XUtils::qCoro(socket).open(url)));
        using namespace std::chrono_literals;
        QCORO_DELAY(socket.sendBinaryMessage("One large, please"));

        auto const frames{ XUtils::qCoro(socket).binaryFrames() };
        QByteArray data{};
        for (auto frame { co_await frames.begin()}, end = frames.end(); frame != end; co_await ++frame) {
            data += std::get<0>(*frame);
            if (std::get<1>(*frame)) { break; } // last
        }

        QCORO_VERIFY(data.size() >= 10 * 1024 * 1024); // 10MB
    }

private Q_SLOTS:
    void init() { m_server_.start(); }

    void cleanup() { m_server_.stop(); }

    addCoroAndThenTests(WaitForOpenWithUrl)
    addCoroAndThenTests(TimeoutOpenWithUrl)
    addCoroAndThenTests(WaitForOpenWithNetworkRequest)
    addTest(DoesntCoawaitOpenedSocket)
    addCoroAndThenTests(Ping)
    addTest(BinaryFrame)
    addTest(BinaryFrameTimeout)
    addTest(BinaryFrameGeneratorEndsOnSocketClose)
    addTest(BinaryMessage)
    addTest(BinaryMessageTimeout)
    addTest(BinaryMessageGeneratorEndsOnSocketClose)
    addTest(TextFrame)
    addTest(TextFrameTimeout)
    addTest(TextFrameGeneratorEndsOnSocketClose)
    addTest(TextMessage)
    addTest(TextMessageTimeout)
    addTest(TextMessageGeneratorEndsOnSocketClose)

    addTest(ReadFragmentedMessage)

private:
    bool connectSocket(QWebSocket &socket) const
    { return XUtils::waitFor(XUtils::qCoro(socket).open(m_server_.url())); }
};

QTEST_GUILESS_MAIN(QCoroWebSocketTest)

#include "qcorowebsocket.moc"
