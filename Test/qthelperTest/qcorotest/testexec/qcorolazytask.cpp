#include <testobject.hpp>
#include <XQtHelper/qcoro/core/qcorotimer.hpp>
#include <XCoroutine/xcorolazytask.hpp>
#include <XQtHelper/qcoro/core/waitfor.hpp>

using namespace std::chrono_literals;

struct QCoroLazyTaskTest
    : QCoro::TestObject<QCoroLazyTaskTest>
{
    Q_OBJECT

    XUtils::XCoroTask<> testSyncLazyCoroutineStarts_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();
        auto constexpr coro {
            [](bool & started) -> XUtils::XCoroLazyTask<> {started = true; co_return; }
        };
        bool started {};
        auto const task { coro(started) };
        QCORO_VERIFY(!started);
        co_await task;
        QCORO_VERIFY(started);
    }

    XUtils::XCoroTask<> testLazyCoroutineStarts_coro(QCoro::TestContext) {
        auto constexpr coro { [](bool & started, bool & resumed) -> XUtils::XCoroLazyTask<>
            { started = true; co_await XUtils::sleepFor(1ms); resumed = true; }
        };
        bool started {},resumed {};
        auto const task { coro(started, resumed) };
        QCORO_VERIFY(!started);
        co_await task;
        QCORO_VERIFY(started);
        QCORO_VERIFY(resumed);
    }

    XUtils::XCoroTask<> testNonVoidSyncLazyCoroutineStarts_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();
        auto constexpr coro {
            [](bool & started) -> XUtils::XCoroLazyTask<int>
            { started = true; co_return 42; }
        };
        bool started {};
        auto const task { coro(started) };
        QCORO_VERIFY(!started);
        auto const result{ co_await task };
        QCORO_VERIFY(started);
        QCORO_COMPARE(result, 42);
    }

    XUtils::XCoroTask<> testNonVoidLazyCoroutineStarts_coro(QCoro::TestContext) {
        auto constexpr coro {
            [](bool & started, bool & resumed) -> XUtils::XCoroLazyTask<int> {
                started = true;
                co_await XUtils::sleepFor(1ms);
                resumed = true;
                co_return 42;
            }
        };

        bool started{},resumed {};
        auto const task { coro(started, resumed) };
        QCORO_VERIFY(!started);
        auto const result{ co_await task};
        QCORO_VERIFY(started);
        QCORO_VERIFY(resumed);
        QCORO_COMPARE(result, 42);
    }

    XUtils::XCoroTask<> testEagerInsideLazy_coro(QCoro::TestContext) {
        auto constexpr coro { []() -> XUtils::XCoroLazyTask<int> {
            auto constexpr interCoro {
                []() -> XUtils::XCoroTask<int> { co_await XUtils::sleepFor(1ms); co_return 42; }
            };
            co_return co_await interCoro();
        }};
        auto const task { coro() };
        auto const result{ co_await task };
        QCORO_COMPARE(result, 42);
    }

    XUtils::XCoroTask<> testThenLazyContinuation_coro(QCoro::TestContext) {

        auto constexpr coro {
            []() -> XUtils::XCoroLazyTask<int> { co_await XUtils::sleepFor(1ms); co_return 42; }
        };

        auto const task {
            coro().then([](int const result) -> XUtils::XCoroLazyTask<QString>
                { co_await XUtils::sleepFor(1ms); co_return QString::number(result); }
            )
        };

        auto const result{ co_await task };
        QCORO_COMPARE(result, QStringLiteral("42"));
    }

    XUtils::XCoroTask<> testThenEagerContinuation_coro(QCoro::TestContext) {
        auto constexpr coro {
            []() -> XUtils::XCoroLazyTask<int> {
                co_await XUtils::sleepFor(1ms);
                co_return 42;
            }
        };

        auto const task { coro().then([](int result) -> XUtils::XCoroTask<int> {
                co_await XUtils::sleepFor(1ms);
                co_return result;
            })
        };

        auto const result{ co_await task };
        QCORO_COMPARE(result, 42);
    }

    XUtils::XCoroTask<> testThenNonCoroutineContinuation_coro(QCoro::TestContext) {
        auto constexpr coro {
            []()-> XUtils::XCoroLazyTask<int> { co_await XUtils::sleepFor(1ms); co_return 42; }
        };

        auto const task { coro().then([](int const result)
                { return QString::number(result); }
            )
        };

        static_assert(std::is_same_v<decltype(task), const XUtils::XCoroLazyTask<QString>>);

        auto const result{ co_await task };
        QCORO_COMPARE(result, QStringLiteral("42"));
    }

private Q_SLOTS:
    addTest(SyncLazyCoroutineStarts)
    addTest(LazyCoroutineStarts)
    addTest(NonVoidSyncLazyCoroutineStarts)
    addTest(NonVoidLazyCoroutineStarts)
    addTest(EagerInsideLazy)
    addTest(ThenLazyContinuation)
    addTest(ThenEagerContinuation)
    addTest(ThenNonCoroutineContinuation)

    void testWaitFor() {
        auto constexpr coro {
            []() -> XUtils::XCoroLazyTask<int> { co_await XUtils::sleepFor(1ms);co_return 42; }
        };

        auto const result{ XUtils::waitFor(coro()) };
        QCOMPARE(result, 42);
    }
};


QTEST_GUILESS_MAIN(QCoroLazyTaskTest)

#include "qcorolazytask.moc"
