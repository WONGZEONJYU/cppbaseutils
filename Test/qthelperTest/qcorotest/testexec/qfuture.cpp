#include <testobject.hpp>
#include <XQtHelper/qcoro/core/qcorofuture.hpp>

#include <QString>
#include <QException>
#include <QtConcurrentRun>
#if QT_VERSION_MAJOR > 6
#include <QPromise>
#endif

#include <thread>

using namespace std::chrono_literals;

class TestException : public QException {
    QString m_msg_ {};
public:
    explicit(false) TestException(QString const & msg)
        : m_msg_{msg}
    {   }

    const char *what() const noexcept override
    { return qUtf8Printable(m_msg_); }

    TestException * clone() const override
    { return std::make_unique<TestException>(m_msg_).release(); }

    void raise() const override
    { throw *this; }
};

struct MoveOnly {
    int m_value {};
    explicit(false) constexpr MoveOnly(int const value)
        : m_value{value} {   }
    Q_DISABLE_COPY(MoveOnly)
    X_DEFAULT_MOVE(MoveOnly)
    ~MoveOnly() = default;
};

struct QCoroFutureTest : QCoro::TestObject<QCoroFutureTest> {
    Q_OBJECT

    XUtils::XCoroTask<> testTriggers_coro(QCoro::TestContext) {
        auto const future { QtConcurrent::run([]()noexcept{ std::this_thread::sleep_for(100ms); }) };
        co_await future;
        QCORO_VERIFY(future.isFinished());
    }

    XUtils::XCoroTask<> testQCoroWrapperTriggers_coro(QCoro::TestContext) {
        auto const future { QtConcurrent::run([]()noexcept{ std::this_thread::sleep_for(100ms); }) };
        co_await XUtils::qCoro(future).waitForFinished();
        QCORO_VERIFY(future.isFinished());
    }

    static void testThenQCoroWrapperTriggers_coro(TestLoop & el) {
        auto const future { QtConcurrent::run([]()noexcept{ std::this_thread::sleep_for(100ms); }) };

        bool called {};
        XUtils::qCoro(future).waitForFinished().then([&]()noexcept{ called = true;el.quit();});
        el.exec();
        QVERIFY(called);
    }

    XUtils::XCoroTask<> testReturnsResult_coro(QCoro::TestContext) {
        auto const result{ co_await QtConcurrent::run([]()noexcept{
                std::this_thread::sleep_for(100ms);
                return QStringLiteral("42");
            })
        };
        QCORO_COMPARE(result, QStringLiteral("42"));
    }

    void testThenReturnsResult_coro(TestLoop & el) {
        auto const future { QtConcurrent::run([]{std::this_thread::sleep_for(100ms);return QStringLiteral("42");}) };
        bool called {};
        auto f {
            [&](QString const & result){
                called = true; el.quit(); QCOMPARE(result, QStringLiteral("42"));
            }
        };

        XUtils::qCoro(future).waitForFinished().then(std::move(f));
        el.exec();
        QVERIFY(called);
    }

    XUtils::XCoroTask<> testDoesntBlockEventLoop_coro(QCoro::TestContext) {
        QCoro::EventLoopChecker const eventLoopResponsive {};
        co_await QtConcurrent::run([]()noexcept{ std::this_thread::sleep_for(500ms); });
        QCORO_VERIFY(eventLoopResponsive);
    }

    XUtils::XCoroTask<> testDoesntCoAwaitFinishedFuture_coro(QCoro::TestContext test) {
        auto const future { QtConcurrent::run([]()noexcept { std::this_thread::sleep_for(100ms); }) };
        co_await future;
        QCORO_VERIFY(future.isFinished());
        test.setShouldNotSuspend();
        co_await future;
    }

    void testThenDoesntCoAwaitFinishedFuture_coro(TestLoop & el) {
        auto const future { QtConcurrent::run([]()noexcept { std::this_thread::sleep_for(1ms); }) };
        QTest::qWait((100ms).count());
        QVERIFY(future.isFinished());

        bool called {};
        XUtils::qCoro(future).waitForFinished().then([&]()noexcept{
            called = true; el.quit();
        });
        el.exec();
        QVERIFY(called);
    }

    XUtils::XCoroTask<> testDoesntCoAwaitCanceledFuture_coro(QCoro::TestContext test) {
        test.setShouldNotSuspend();
        QFuture<void> const future{};
        co_await future;
    }

    static void testThenDoesntCoAwaitCanceledFuture_coro(TestLoop &el) {
        QFuture<void> const future {};
        bool called {};
        XUtils::qCoro(future).waitForFinished().then([&]()noexcept{
            called = true;
            el.quit();
        });
        el.exec();
        QVERIFY(called);
    }

    XUtils::XCoroTask<> testPropagateQExceptionFromVoidConcurrent_coro(QCoro::TestContext) {
        auto const future { QtConcurrent::run([]{
            std::this_thread::sleep_for(100ms);
            throw TestException(QStringLiteral("Ooops"));
        })};
        QCORO_VERIFY_EXCEPTION_THROWN(co_await future, TestException);
    }

    XUtils::XCoroTask<> testPropagateQExceptionFromNonvoidConcurrent_coro(QCoro::TestContext) {
        auto throwException {true};
        auto const future { QtConcurrent::run([throwException]{
                std::this_thread::sleep_for(100ms);
                if (throwException) { // Workaround MSVC reporting the "return" stmt as unreachablet
                    throw TestException(QStringLiteral("Ooops"));
                }
                return 42;
            })
        };
        QCORO_VERIFY_EXCEPTION_THROWN(co_await future, TestException);
    }

#if QT_VERSION_MAJOR >= 6
    XUtils::XCoroTask<> testPropagateQExceptionFromVoidPromise_coro(QCoro::TestContext) {
        QPromise<void> promise{};
        QTimer::singleShot(100ms, this, [&promise]{
            promise.start();
            promise.setException(TestException(QStringLiteral("Booom")));
            promise.finish();
        });

        QCORO_VERIFY_EXCEPTION_THROWN(co_await promise.future(), TestException);
    }

    XUtils::XCoroTask<> testPropagateQExceptionFromNonvoidPromise_coro(QCoro::TestContext) {
        QPromise<int> promise{};
        QTimer::singleShot(100ms, this, [&promise]{
            promise.start();
            promise.setException(TestException(QStringLiteral("Booom")));
            promise.finish();
        });

        QCORO_VERIFY_EXCEPTION_THROWN(co_await promise.future(), TestException);
    }

    XUtils::XCoroTask<> testPropagateStdExceptionFromVoidPromise_coro(QCoro::TestContext) {
        QPromise<void> promise{};
        QTimer::singleShot(100ms, this, [&promise]{
            promise.start();
            promise.setException(std::make_exception_ptr(std::runtime_error("Booom")));
            promise.finish();
        });

        QCORO_VERIFY_EXCEPTION_THROWN(co_await promise.future(), std::runtime_error);
    }

    XUtils::XCoroTask<> testPropagateStdExceptionFromNonvoidPromise_coro(QCoro::TestContext) {
        QPromise<void> promise{};
        QTimer::singleShot(100ms, this, [&promise] {
            promise.start();
            promise.setException(std::make_exception_ptr(std::runtime_error("Booom")));
            promise.finish();
        });

        QCORO_VERIFY_EXCEPTION_THROWN(co_await promise.future(), std::runtime_error);
    }

    XUtils::XCoroTask<> testTakeResult_coro(QCoro::TestContext) {
        auto const future { QtConcurrent::run([]{
            std::this_thread::sleep_for(10ms);
            return MoveOnly(42);
        })};

        auto const result{ co_await XUtils::qCoro(future).takeResult() };
        QCORO_COMPARE(result.m_value, 42);

        QPromise<MoveOnly> promise{};
        QTimer::singleShot(10ms, this, [&promise]{
            promise.start();
            promise.addResult(MoveOnly(84));
            promise.finish();
        });

        QCORO_COMPARE((co_await XUtils::qCoro(promise.future()).takeResult()).m_value, 84);
    }

    static void testThenTakeResult_coro(TestLoop & el) {
        auto const future{
            QtConcurrent::run([]{
                std::this_thread::sleep_for(10ms);
                return MoveOnly(42);
            })
        };

        bool called {};
        XUtils::qCoro(future).takeResult().then([&](MoveOnly const result) {
            called = true;
            QCOMPARE(result.m_value, 42);
            el.quit();
        });
        el.exec();
        QVERIFY(called);
    }

#endif

// QPromise cancelling running future on destruction has been introduced in
// Qt 6.3.
#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 1)
    XUtils::XCoroTask<> testUnfinishedPromiseDestroyed_coro(QCoro::TestContext) {
        const auto future { [this]{
                auto promise { std::make_shared<QPromise<int>>()};
                auto future_ { promise->future()};
                QTimer::singleShot(400ms, this, [p = promise] {p->addResult(42);});
                return future_;
            }()
        };
        co_await future;
    }
#endif

private Q_SLOTS:
    addTest(Triggers)
    addCoroAndThenTests(ReturnsResult)
    addTest(DoesntBlockEventLoop)
    addCoroAndThenTests(DoesntCoAwaitFinishedFuture)
    addCoroAndThenTests(DoesntCoAwaitCanceledFuture)
    addCoroAndThenTests(QCoroWrapperTriggers)
    addTest(PropagateQExceptionFromVoidConcurrent)
    addTest(PropagateQExceptionFromNonvoidConcurrent)
#if QT_VERSION_MAJOR >= 6
    addTest(PropagateQExceptionFromVoidPromise)
    addTest(PropagateQExceptionFromNonvoidPromise)
    addTest(PropagateStdExceptionFromVoidPromise)
    addTest(PropagateStdExceptionFromNonvoidPromise)
    addCoroAndThenTests(TakeResult)
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 1)
    addTest(UnfinishedPromiseDestroyed)
#endif
};

QTEST_GUILESS_MAIN(QCoroFutureTest)

#include "qfuture.moc"
