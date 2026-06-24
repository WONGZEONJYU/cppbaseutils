#include <testobject.hpp>

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)

#include <XQtHelper/qcoro/core/qcorochronotimer.hpp>
#include <chrono>
#include <QElapsedTimer>

using namespace std::chrono_literals;

struct QCoroChronoTimerTest : QCoro::TestObject<QCoroChronoTimerTest> {
    Q_OBJECT

    XUtils::XCoroTask<> testTriggers_coro(QCoro::TestContext) {
        QChronoTimer timer {};
        timer.setInterval(100ms);
        timer.start();
        co_await timer;
    }

    XUtils::XCoroTask<> testQCoroWrapperTriggers_coro(QCoro::TestContext) {
        QChronoTimer timer {};
        timer.setInterval(100ms);
        timer.start();
        co_await XUtils::qCoro(timer).waitForTimeout();
    }

    XUtils::XCoroTask<> testDoesntBlockEventLoop_coro(QCoro::TestContext) {
        QCoro::EventLoopChecker eventLoopResponsive{};

        QChronoTimer timer {};
        timer.setInterval(500ms);
        timer.start();

        co_await timer;

        QCORO_VERIFY(eventLoopResponsive);
    }

    XUtils::XCoroTask<> testDoesntCoAwaitInactiveTimer_coro(QCoro::TestContext ctx) {
        ctx.setShouldNotSuspend();
        QChronoTimer timer {};
        timer.setInterval(1s);
        // Don't start the timer!

        co_await timer;
    }

    XUtils::XCoroTask<> testDoesntCoAwaitNullTimer_coro(QCoro::TestContext ctx) {
        ctx.setShouldNotSuspend();

        QChronoTimer * timer {};

        co_await timer;
    }

    void testThenTriggers_coro(TestLoop & el) {
        QChronoTimer timer {};
        bool triggered {};
        timer.setInterval(10ms);
        timer.start();
        XUtils::qCoro(timer).waitForTimeout().then([&el, &triggered]() {
            triggered = true;
            el.quit();
        });
        el.exec();
        QVERIFY(triggered);
    }

    XUtils::XCoroTask<> testSleepFor_coro(QCoro::TestContext) {
        QElapsedTimer elapsed{};
        elapsed.start();
        co_await XUtils::chronoSleepFor(100ms);
        QCORO_VERIFY(elapsed.elapsed() >= 75);
    }

    XUtils::XCoroTask<> testSleepUntil_coro(QCoro::TestContext) {
        QElapsedTimer elapsed{};
        elapsed.start();
        co_await XUtils::chronoSleepUntil(std::chrono::steady_clock::now() + 500ms);
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

QTEST_GUILESS_MAIN(QCoroChronoTimerTest)

#include "qchronotimer.moc"

#else

int main([[maybe_unused]]int argc, [[maybe_unused]] char *argv[]) {
    qDebug() << R"(Not Support QChronoTimer!)";
    return 0;
}

#endif
