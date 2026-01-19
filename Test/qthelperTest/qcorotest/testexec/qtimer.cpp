#include <testobject.hpp>
#include <XQtHelper/qcoro/core/qcorotimer.hpp>
#include <chrono>
#include <QElapsedTimer>

using namespace std::chrono_literals;

struct QCoroTimerTest : QCoro::TestObject<QCoroTimerTest> {
    Q_OBJECT

    XUtils::XCoroTask<> testTriggers_coro(QCoro::TestContext) {
        QTimer timer {};
        timer.setInterval(100ms);
        timer.start();
        co_await timer;
    }

    XUtils::XCoroTask<> testQCoroWrapperTriggers_coro(QCoro::TestContext) {
        QTimer timer{};
        timer.setInterval(100ms);
        timer.start();
        co_await XUtils::qCoro(timer).waitForTimeout();
    }

    XUtils::XCoroTask<> testDoesntBlockEventLoop_coro(QCoro::TestContext) {
        QCoro::EventLoopChecker eventLoopResponsive;

        QTimer timer;
        timer.setInterval(500ms);
        timer.start();

        co_await timer;

        QCORO_VERIFY(eventLoopResponsive);
    }

    XUtils::XCoroTask<> testDoesntCoAwaitInactiveTimer_coro(QCoro::TestContext ctx) {
        ctx.setShouldNotSuspend();

        QTimer timer;
        timer.setInterval(1s);
        // Don't start the timer!

        co_await timer;
    }

    XUtils::XCoroTask<> testDoesntCoAwaitNullTimer_coro(QCoro::TestContext ctx) {
        ctx.setShouldNotSuspend();

        QTimer *timer = nullptr;

        co_await timer;
    }

    void testThenTriggers_coro(TestLoop & el) {
        QTimer timer{};
        bool triggered = false;
        timer.start(10ms);
        XUtils::qCoro(timer).waitForTimeout().then([&el, &triggered]() {
            triggered = true;
            el.quit();
        });
        el.exec();
        QVERIFY(triggered);
    }

    XUtils::XCoroTask<> testSleepFor_coro(QCoro::TestContext) {
        QElapsedTimer elapsed;
        elapsed.start();
        co_await XUtils::sleepFor(100ms);
        QCORO_VERIFY(elapsed.elapsed() >= 75);
    }

    XUtils::XCoroTask<> testSleepUntil_coro(QCoro::TestContext) {
        QElapsedTimer elapsed;
        elapsed.start();
        co_await XUtils::sleepUntil(std::chrono::steady_clock::now() + 500ms);
        QCORO_VERIFY(elapsed.elapsed() >= 475);
    }

private Q_SLOTS:
    addTest(Triggers)
    addTest(QCoroWrapperTriggers)
    addTest(DoesntBlockEventLoop)
    addTest(DoesntCoAwaitInactiveTimer)
    addTest(DoesntCoAwaitNullTimer)
    addTest(SleepFor)
    addTest(SleepUntil)
    addThenTest(Triggers)
};

QTEST_GUILESS_MAIN(QCoroTimerTest)

#include "qtimer.moc"
