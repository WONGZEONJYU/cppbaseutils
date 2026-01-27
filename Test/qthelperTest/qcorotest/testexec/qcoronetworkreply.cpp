#include <testhttpserver.hpp>
#include <testobject.hpp>
#include "qcoroiodevice_macros.hpp"
#include <XQtHelper/qcoro/network/qcoronetworkreply.hpp>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QTcpServer>

struct QCoroNetworkReplyTest : QCoro::TestObject<QCoroNetworkReplyTest> {
    Q_OBJECT
    TestHttpServer<QTcpServer> m_server_{};

    XUtils::XCoroTask<> testTriggers_coro(QCoro::TestContext) {
        QNetworkAccessManager nam {};
        auto const reply {std::unique_ptr<QNetworkReply>(nam.get(buildRequest()))};
        (void)co_await reply.get();
        QCORO_VERIFY(reply->isFinished());
        QCORO_COMPARE(reply->error(), QNetworkReply::NoError);
        QCORO_COMPARE(reply->readAll(), "abcdef");
    }

    XUtils::XCoroTask<> testQCoroWrapperTriggers_coro(QCoro::TestContext) {
        QNetworkAccessManager nam{};
        auto const reply { std::unique_ptr<QNetworkReply>(nam.get(buildRequest())) };

        co_await XUtils::qCoro(reply.get()).waitForFinished();

        QCORO_VERIFY(reply->isFinished());
        QCORO_COMPARE(reply->error(), QNetworkReply::NoError);
        QCORO_COMPARE(reply->readAll(), "abcdef");
    }

    void testThenQCoroWrapperTriggers_coro(TestLoop &el) {
        QNetworkAccessManager nam{};
        auto const reply { std::unique_ptr<QNetworkReply>(nam.get(buildRequest())) };

        bool called {};
        XUtils::qCoro(reply.get()).waitForFinished().then([&](bool const finished) {
            called = true;
            el.quit();
            QVERIFY(finished);
        });
        el.exec();
        QVERIFY(reply->isFinished());
        QCOMPARE(reply->error(), QNetworkReply::NoError);
        QCOMPARE(reply->readAll(), "abcdef");
        QVERIFY(called);
    }

    XUtils::XCoroTask<> testDoesntBlockEventLoop_coro(QCoro::TestContext) {
        QCoro::EventLoopChecker const eventLoopResponsive{};
        QNetworkAccessManager nam{};
        auto const reply { std::unique_ptr<QNetworkReply>(nam.get(buildRequest(QStringLiteral("block")))) };
        (void)co_await reply.get();
        QCORO_VERIFY(eventLoopResponsive);
        QCORO_VERIFY(reply->isFinished());
        QCORO_COMPARE(reply->error(), QNetworkReply::NoError);
        QCORO_COMPARE(reply->readAll(), "abcdef");
    }

    XUtils::XCoroTask<> testDoesntCoAwaitNullReply_coro(QCoro::TestContext test) {
        test.setShouldNotSuspend();
        m_server_.setExpectTimeout(true);
        QNetworkReply *reply {};
        (void)co_await reply;
        delete reply;
    }

    XUtils::XCoroTask<> testDoesntCoAwaitFinishedReply_coro(QCoro::TestContext test) {
        QNetworkAccessManager nam{};
        auto const reply { std::unique_ptr<QNetworkReply>(nam.get(buildRequest())) };
        (void)co_await reply.get();
        QCORO_VERIFY(reply->isFinished());
        test.setShouldNotSuspend();
        (void)co_await reply.get();
    }

    XUtils::XCoroTask<> testReadAllTriggers_coro(QCoro::TestContext) {
        QNetworkAccessManager nam{};
        auto const reply { std::unique_ptr<QNetworkReply>(nam.get(buildRequest(QStringLiteral("stream")))) };
        using namespace XUtils;
        QCORO_TEST_IODEVICE_READALL(*reply);
        QCORO_COMPARE(data.size(), reply->rawHeader("Content-Length").toInt());
    }

    void testThenReadAllTriggers_coro(TestLoop &el) {
        QNetworkAccessManager nam{};
        auto const reply { std::unique_ptr<QNetworkReply>(nam.get(buildRequest(QStringLiteral("block")))) };
        bool called {};
        XUtils::qCoro(reply.get()).readAll().then(
        [&](QByteArray const & data){called = true;el.quit();QVERIFY(!data.isEmpty());}
        );
        el.exec();
        QVERIFY(called);
    }

    XUtils::XCoroTask<> testReadTriggers_coro(QCoro::TestContext) {
        QNetworkAccessManager nam{};
        auto const reply { std::unique_ptr<QNetworkReply>(nam.get(buildRequest(QStringLiteral("stream"))))};
        using namespace XUtils;
        QCORO_TEST_IODEVICE_READ(*reply);
        QCORO_COMPARE(data.size(), reply->rawHeader("Content-Length").toInt());
    }

    void testThenReadTriggers_coro(TestLoop &el) {
        QNetworkAccessManager nam{};
        auto const reply { std::unique_ptr<QNetworkReply>(nam.get(buildRequest(QStringLiteral("block")))) };
        bool called {};
        XUtils::qCoro(reply.get()).read(1).then(
        [&](QByteArray const & data){ called = true;el.quit();QCOMPARE(data.size(), 1); }
        );
        el.exec();
        QVERIFY(called);
    }

    XUtils::XCoroTask<> testReadLineTriggers_coro(QCoro::TestContext) {
        QNetworkAccessManager nam;
        auto const reply { std::unique_ptr<QNetworkReply>(nam.get(buildRequest(QStringLiteral("stream")))) };
        using namespace XUtils;
        QCORO_TEST_IODEVICE_READLINE(*reply);
        QCORO_COMPARE(lines.size(), 10);
    }

    void testThenReadLineTriggers_coro(TestLoop &el) {
        QNetworkAccessManager nam{};
        auto const reply { std::unique_ptr<QNetworkReply>(nam.get(buildRequest(QStringLiteral("block")))) };
        bool called {};
        XUtils::qCoro(reply.get()).readLine().then(
            [&](QByteArray const & data) {called = true;el.quit();QVERIFY(!data.isEmpty());}
        );
        el.exec();
        QVERIFY(called);
    }

    // See https://github.com/danvratil/qcoro/issues/231
    XUtils::XCoroTask<> testAbortOnTimeout_coro(QCoro::TestContext) {
        auto request { buildRequest(QStringLiteral("block"))};
        request.setTransferTimeout(300);
        QNetworkAccessManager nam{};
        auto const reply { co_await nam.get(request) };
        QCORO_VERIFY(reply != nullptr);
        QCORO_VERIFY(reply->isFinished());
        // Seems to depend on the Qt version which error we get.
        QCORO_VERIFY(reply->error() == QNetworkReply::TimeoutError || reply->error() == QNetworkReply::OperationCanceledError);
        // QNAM is destroyed here and so is all its associated state, which could
        // crash (or cause invalid memory access)
    }

private Q_SLOTS:
    void init() { m_server_.start(QHostAddress::LocalHost); }

    void cleanup() { m_server_.stop(); }

    addTest(Triggers)
    addCoroAndThenTests(QCoroWrapperTriggers)
    addTest(DoesntBlockEventLoop)
    addTest(DoesntCoAwaitNullReply)
    addTest(DoesntCoAwaitFinishedReply)
    addCoroAndThenTests(ReadAllTriggers)
    addCoroAndThenTests(ReadTriggers)
    addCoroAndThenTests(ReadLineTriggers)
    addTest(AbortOnTimeout)

private:
    QNetworkRequest buildRequest(QString const & path = {}) const
    { return QNetworkRequest{ QUrl{QStringLiteral("http://127.0.0.1:%1/%2").arg(m_server_.port()).arg(path)} }; }
};

QTEST_GUILESS_MAIN(QCoroNetworkReplyTest)

#include "qcoronetworkreply.moc"
