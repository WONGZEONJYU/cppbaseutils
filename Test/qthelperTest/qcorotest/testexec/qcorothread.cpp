#include <testobject.hpp>
#include <testmacros.hpp>

#include <XQtHelper/qcoro/core/qcorothread.hpp>
#include <XQtHelper/qcoro/core/qcorosignal.hpp>

#include <QThread>
#include <QScopeGuard>

#include <thread>
#include <memory>

using namespace std::chrono_literals;

struct QCoroThreadTest : QCoro::TestObject<QCoroThreadTest> {
    Q_OBJECT

    XUtils::XCoroTask<> testWaitForStarted_coro(QCoro::TestContext) {

        std::unique_ptr<QThread> const thread(QThread::create([]{
            std::this_thread::sleep_for(100ms);
        }));

        auto const threadGuard { qScopeGuard([&]{ thread->wait(); }) };

        QCORO_DELAY(thread->start());

        auto const ok{ co_await XUtils::qCoro(thread.get()).waitForStarted()};
        QCORO_VERIFY(thread->isRunning());
        QCORO_VERIFY(ok);
    }

    XUtils::XCoroTask<> testWaitForFinished_coro(QCoro::TestContext) {
        std::unique_ptr<QThread> const thread(QThread::create([]{
            std::this_thread::sleep_for(100ms);
        }));

        thread->start();
        co_await XUtils::qCoro(thread.get()).waitForStarted();
        QCORO_VERIFY(thread->isRunning());
        auto const ok { co_await XUtils::qCoro(thread.get()).waitForFinished() };
        QCORO_VERIFY(thread->isFinished());

        QCORO_VERIFY(ok);
    }

    XUtils::XCoroTask<> testMoveToThread_coro(QCoro::TestContext) {
        QThread newThread{};
        newThread.start();

        QCORO_COMPARE(QThread::currentThread(), QCoreApplication::instance()->thread());

        co_await XUtils::moveToThread(&newThread);

        QCORO_COMPARE(QThread::currentThread(), &newThread);

        co_await XUtils::moveToThread(qApp->thread());

        QCORO_COMPARE(QThread::currentThread(), QCoreApplication::instance()->thread());

        newThread.exit();
        newThread.wait();
    }

private Q_SLOTS:
    addTest(WaitForStarted)
    addTest(WaitForFinished)
    addTest(MoveToThread)
};


QTEST_GUILESS_MAIN(QCoroThreadTest)

#include "qcorothread.moc"
