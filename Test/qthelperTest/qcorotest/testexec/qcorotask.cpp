#include <testobject.hpp>
#include <XQtHelper/qcoro/core/qcorotimer.hpp>
#include <XQtHelper/qcoro/core/connect.hpp>
#include <chrono>
#include <QTest>
#include <QObject>
#include <QScopeGuard>
#include <QMetaObject>
#include <QTimer>
#include <QElapsedTimer>

using namespace std::chrono_literals;

namespace {

XUtils::XCoroTask<> timer(std::chrono::milliseconds const timeout = 10ms) {
    QTimer timer{};
    timer.setSingleShot(true);
    timer.start(timeout);
    co_await timer;
}

template<typename T>
XUtils::XCoroTask<T> timerWithValue(T value, std::chrono::milliseconds const timeout = 10ms)
{ co_await timer(timeout); co_return value; }

XUtils::XCoroTask<> thenScopeTestFunc(QEventLoop *el)
{ return timer().then([el] {el->quit();}); }

template<typename T>
XUtils::XCoroTask<T> thenScopeTestFuncWithValue(T && value)
{ return timer().then([value = std::forward<T>(value)]{ return value;}); }

struct ImplicitConversionBar
{ int m_number; };

struct ImplicitConversionFoo {
    constexpr ImplicitConversionFoo() = default;
    explicit(false) ImplicitConversionFoo(ImplicitConversionBar const bar)
        : m_string_(QString::number(bar.m_number)) {}

    QString m_string_{};
};

struct TestAwaitableBase {
    [[nodiscard]] auto delay() const noexcept{ return m_delay_; }
private:
    std::chrono::milliseconds m_delay_ { 100ms };
};

template<typename T>
struct TestAwaitable : TestAwaitableBase {

    explicit(false) TestAwaitable(T const val) : m_result_(val) {  }

    static constexpr bool await_ready() noexcept { return {}; }

    static constexpr void await_suspend(std::coroutine_handle<> const h) noexcept
    { QTimer::singleShot(100ms, [h] { h.resume();}); }

    [[nodiscard]] T await_resume() const { return m_result_; }

private:
    T m_result_{};
};

template<>
struct TestAwaitable<void> : TestAwaitableBase {

    static constexpr bool await_ready() noexcept { return {}; }

    static void await_suspend(std::coroutine_handle<> const h) noexcept
    { QTimer::singleShot(100ms, [h] { h.resume(); }); }

    static constexpr void await_resume() noexcept {}
};

template<typename T = void>
struct TestAwaitableWithCoAwait {
    explicit(false) TestAwaitableWithCoAwait(T const val) : m_result_(val) {    }

    auto operator co_await()const noexcept
    { return TestAwaitable(m_result_); }

private:
    T m_result_{};
};

template<>
struct TestAwaitableWithCoAwait<void> {
    auto operator co_await() const noexcept
    { return TestAwaitable<void>(); }
};

} // namespace

class QCoroTaskTest : public QCoro::TestObject<QCoroTaskTest>
{
    Q_OBJECT

    template<typename Coro>
    void ignoreCoroutineResult(QEventLoop & el, Coro && coro) {
        QTimer::singleShot(5s, &el, [&el] { el.exit(1); });

        std::forward<Coro>(coro)();
        const int timeout = el.exec();
        QCOMPARE(timeout, 0);
    }

    XUtils::XCoroTask<> testSimpleCoroutine_coro(QCoro::TestContext)
    { co_await timer(); }

    XUtils::XCoroTask<> testCoroutineValue_coro(QCoro::TestContext) {
        auto constexpr coro {
            [](QString const & result) -> XUtils::XCoroTask<QString>
            { co_await timer(); co_return result; }
        };

        auto const value { QStringLiteral("Done!") },result{ co_await coro(value)};
        QCORO_COMPARE(result, value);
    }

    XUtils::XCoroTask<> testCoroutineMoveValue_coro(QCoro::TestContext) {

        auto constexpr coro {
            [](QString const & result) -> XUtils::XCoroTask<std::unique_ptr<QString>> {
                co_await timer();
                co_return std::make_unique<QString>(result);
            }
        };

        auto const value { QStringLiteral("Done ptr!") };
        auto const result{ co_await coro(value)};
        QCORO_COMPARE(*result.get(), value);
    }

    XUtils::XCoroTask<> testSyncCoroutine_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();
        auto constexpr coro { []()-> XUtils::XCoroTask<int>{ co_return 42; } };
        const auto result{ co_await coro()};
        QCORO_COMPARE(result, 42);
    }

    XUtils::XCoroTask<> testCoroutineWithException_coro(QCoro::TestContext) {
        auto constexpr coro {
            []()-> XUtils::XCoroTask<int> {
                co_await timer();
                throw std::runtime_error("Invalid result");
                co_return 42;
            }
        };

        try {
            auto const result{ co_await coro() };
            QCORO_FAIL("Exception was not propagated.");
            Q_UNUSED(result);
        } catch (std::runtime_error const & e) {
            // OK
            qDebug() << e.what();
        } catch (...) {
            QCORO_FAIL("Exception type was not propagated, or other exception was thrown.");
        }
    }

    XUtils::XCoroTask<> testVoidCoroutineWithException_coro(QCoro::TestContext) {
        auto constexpr coro {
            []() -> XUtils::XCoroTask<> { co_await timer(); throw std::runtime_error("Error"); }
        };

        try {
            co_await coro();
            QCORO_FAIL("Exception was not propagated.");
        } catch (std::runtime_error const & e) {
            // OK
            qDebug() << e.what();
        } catch (...) {
            QCORO_FAIL("Exception type was not propagated, or other exception was thrown.");
        }
    }

    XUtils::XCoroTask<> testCoroutineFrameDestroyed_coro(QCoro::TestContext) {
        bool destroyed {};
        auto const coro {
            [&destroyed]() -> XUtils::XCoroTask<> {
                auto const guard { qScopeGuard([&destroyed]{destroyed = true; }) };
                QCORO_VERIFY(!destroyed);
                co_await timer();
                QCORO_VERIFY(!destroyed);
            }
        };

        co_await coro();
        QCORO_VERIFY(destroyed);
    }

    XUtils::XCoroTask<> testExceptionPropagation_coro(QCoro::TestContext) {
        QCORO_VERIFY_EXCEPTION_THROWN(
            co_await []() -> XUtils::XCoroTask<int> {
                throw std::runtime_error("Test!");
                co_return 42;
            }(),std::runtime_error);

        QCORO_VERIFY_EXCEPTION_THROWN(
            co_await []() -> XUtils::XCoroTask<> {
                throw std::runtime_error("Test!");
            }(),std::runtime_error);

        QCORO_VERIFY_EXCEPTION_THROWN(
            co_await []() -> XUtils::XCoroTask<int> {
                co_await timer();
                throw std::runtime_error("Test!");
                co_return 42;
            }(),std::runtime_error);

        QCORO_VERIFY_EXCEPTION_THROWN(
            co_await []() -> XUtils::XCoroTask<> {
                co_await timer();
                throw std::runtime_error("Test!");
            }(),std::runtime_error);
    }

    XUtils::XCoroTask<> testThenReturnValueNoArgument_coro(QCoro::TestContext) {
        auto task { timer().then([]{return 42;}) };
        static_assert(std::is_same_v<decltype(task), XUtils::XCoroTask<int>>);
        auto const result{ co_await task };
        QCORO_COMPARE(result, 42);
    }

    XUtils::XCoroTask<> testThenReturnValueWithArgument_coro(QCoro::TestContext) {
        auto constexpr f { [](int const param) -> XUtils::XCoroTask<int> { co_return param * 2; } };
        auto task { timerWithValue(42).then(f) };
        static_assert(std::is_same_v<decltype(task), XUtils::XCoroTask<int>>);
        auto const result{ co_await task };
        QCORO_COMPARE(result, 84);
    }

    XUtils::XCoroTask<> testThenReturnTaskVoidNoArgument_coro(QCoro::TestContext) {
        auto task { timer().then([]()-> XUtils::XCoroTask<> { co_await timer(); }) };
        static_assert(std::is_same_v<decltype(task), XUtils::XCoroTask<>>);
        co_await task;
    }

    XUtils::XCoroTask<> testThenReturnTaskVoidWithArgument_coro(QCoro::TestContext) {
        auto constexpr f { [](int) -> XUtils::XCoroTask<> { co_await timer(); } };
        auto task { timerWithValue(42).then(f) };
        static_assert(std::is_same_v<decltype(task), XUtils::XCoroTask<>>);
        co_await task;
    }

    XUtils::XCoroTask<> testThenReturnTaskTNoArgument_coro(QCoro::TestContext) {
        auto constexpr f { []() -> XUtils::XCoroTask<int> { co_await timer(); co_return 42; } };
        auto task { timer().then(f)};
        static_assert(std::is_same_v<decltype(task), XUtils::XCoroTask<int>>);
        auto const result{ co_await task };
        QCORO_COMPARE(result, 42);
    }

    XUtils::XCoroTask<> testThenReturnTaskTWithArgument_coro(QCoro::TestContext) {
        auto const f { [](int const val) -> XUtils::XCoroTask<int> { co_await timer(); co_return val * 2; } };
        auto task { timerWithValue(42).then(f) };
        static_assert(std::is_same_v<decltype(task), XUtils::XCoroTask<int>>);
        auto const result{ co_await task };
        QCORO_COMPARE(result, 84);
    }

    XUtils::XCoroTask<> testThenReturnValueSync_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();
        auto task {
            []()-> XUtils::XCoroTask<int> {co_return 42;}().then([](int const param){ return param * 2; })
        };
        auto const result{ co_await task };
        QCORO_COMPARE(result, 84);
    }

    XUtils::XCoroTask<> testThenScopeAwait_coro(QCoro::TestContext) {
        auto const result{ co_await thenScopeTestFuncWithValue(42)};
        QCORO_COMPARE(result, 42);
    }

    XUtils::XCoroTask<> testThenExceptionPropagation_coro(QCoro::TestContext) {
        QCORO_VERIFY_EXCEPTION_THROWN(
            co_await []() -> XUtils::XCoroTask<int> {
                co_await timer();
                throw std::runtime_error("Test!");
                co_return 42;
            }().then([](int) -> XUtils::XCoroTask<>  {
                QCORO_FAIL("The then() callback should never be called");
                co_return;
            }),
            std::runtime_error);
    }

    XUtils::XCoroTask<>  testThenError_coro(QCoro::TestContext) {
        bool exceptionThrown {};
        co_await []() -> XUtils::XCoroTask<int> {
            co_await timer();
            throw std::runtime_error("Test!");
            co_return 42;
        }().then([](int) -> XUtils::XCoroTask<> {
                QCORO_FAIL("The then() callback should not be called");
            },[&exceptionThrown](std::exception const &){
                exceptionThrown = true;
            }
        );

        QCORO_VERIFY(exceptionThrown);
    }

    XUtils::XCoroTask<> testThenErrorWithValue_coro(QCoro::TestContext) {
        bool exceptionThrown {},thenCalled {};
        auto const result{
            co_await []() -> XUtils::XCoroTask<>{
                co_await timer();
                throw std::runtime_error("Test!");
            }().then([&thenCalled]() -> XUtils::XCoroTask<int>
                { thenCalled = true;co_return 42;}
                ,[&exceptionThrown](const std::exception &) {exceptionThrown = true; }
            )
        };

        // We handled an exception, so there's no error and it should
        // be default-constructed.
        QCORO_COMPARE(result, 0);
        QCORO_VERIFY(!thenCalled);
        QCORO_VERIFY(exceptionThrown);
    }

    static void testThenImplicitArgumentConversion_coro(TestLoop & el) {
        QTimer test {};
        QString result {};

        auto constexpr coroF { []() -> XUtils::XCoroTask<ImplicitConversionBar> {
            ImplicitConversionBar constexpr bar{42};
            co_await timer(10ms);
            co_return bar;
        } };

        auto const cb {
                [&el,&result](ImplicitConversionFoo const & foo){
                result = foo.m_string_;
                el.quit();
            }
        };

        XUtils::qCoro(test).waitForTimeout().then(coroF).then(cb);
        test.start(10ms);
        el.exec();

        QCOMPARE(result, QStringLiteral("42"));
    }

    static void testReturnValueImplicitConversion(QCoro::TestContext) {
        [[maybe_unused]] auto constexpr testcoro { []()-> XUtils::XCoroTask<int> { co_return 42LL; } };
    }

    XUtils::XCoroTask<> testMultipleAwaiters_coro(QCoro::TestContext) {
        auto task { timer(100ms)};
        bool called {};
        // Internally co_awaits task
        task.then([&called] { called = true; });
        co_await task;
        QCORO_VERIFY(called);
    }

    XUtils::XCoroTask<> testMultipleAwaitersSync_coro(QCoro::TestContext ctx) {
        ctx.setShouldNotSuspend();
        auto task { []()-> XUtils::XCoroTask<> { co_return; } () };
        bool called {};
        task.then([&called]{ called = true; });
        co_await task;
        QCORO_VERIFY(called);
    }

    Q_SIGNAL void callbackCalled();

    template <typename QObjectDerived, typename Signal>
    XUtils::XCoroTask<> verifySignalEmitted(QObjectDerived * const context, Signal &&signal) {
        bool called = false;
        co_await XUtils::qCoro(context, std::forward<Signal>(signal)).then([&]{
            called = true;
        });
        QCORO_VERIFY(called);
    }

    XUtils::XCoroTask<> testTaskConnect_coro(QCoro::TestContext) {
        // Test that free functions can be passed as callback
        XUtils::connect(timer(), this, [this]{Q_EMIT callbackCalled(); });
        co_await verifySignalEmitted(this, &QCoroTaskTest::callbackCalled);

        // Check that member functions can be passed as callback
        XUtils::connect(timer(), this, &QCoroTaskTest::callbackCalled);
        co_await verifySignalEmitted(this, &QCoroTaskTest::callbackCalled);

        // Test that the code still compiles if the value of the coroutine is not used by the function.
        auto constexpr nonVoidCoroutine { []() -> XUtils::XCoroTask<QString> { co_await timer(); co_return QStringLiteral("Hello World!"); } };
        XUtils::connect(nonVoidCoroutine(), this, [this] { Q_EMIT callbackCalled(); });
        co_await verifySignalEmitted(this, &QCoroTaskTest::callbackCalled);
        XUtils::connect(nonVoidCoroutine(), this, [this](QString const &){Q_EMIT callbackCalled(); });
        co_await verifySignalEmitted(this, &QCoroTaskTest::callbackCalled);
    }

private Q_SLOTS:
    addTest(SimpleCoroutine)
    addTest(CoroutineValue)
    addTest(CoroutineMoveValue)
    addTest(SyncCoroutine)
    addTest(CoroutineWithException)
    addTest(VoidCoroutineWithException)
    addTest(CoroutineFrameDestroyed)
    addTest(ExceptionPropagation)
    addTest(ThenReturnValueNoArgument)
    addTest(ThenReturnValueWithArgument)
    addTest(ThenReturnTaskVoidNoArgument)
    addTest(ThenReturnTaskVoidWithArgument)
    addTest(ThenReturnTaskTNoArgument)
    addTest(ThenReturnTaskTWithArgument)
    addTest(ThenReturnValueSync)
    addTest(ThenScopeAwait)
    addTest(ThenExceptionPropagation)
    addTest(ThenError)
    addTest(ThenErrorWithValue)
    addTest(TaskConnect)
    addThenTest(ImplicitArgumentConversion)
    addTest(MultipleAwaiters)
    addTest(MultipleAwaitersSync)
#if 1
    // See https://github.com/danvratil/qcoro/issues/24
    void testEarlyReturn()
    {
        QEventLoop loop;

        auto constexpr testReturn { [](bool const immediate) -> XUtils::XCoroTask<bool> {
            if (immediate) { co_return true; }
            co_await timer();
            co_return true;
        }};

        bool immediateResult{},delayedResult {};

        auto const testImmediate {
            [&]() ->XUtils::XCoroTask<> { immediateResult = co_await testReturn(true); }
        };

        auto const testDelayed {
            [&]() -> XUtils::XCoroTask<> {
                delayedResult = co_await testReturn(false);
                loop.quit();
            }
        };

        QMetaObject::invokeMethod(
            &loop, [&]{ testImmediate(); }, Qt::QueuedConnection);
        QMetaObject::invokeMethod(
            &loop, [&]{ testDelayed(); }, Qt::QueuedConnection);

        loop.exec();

        QVERIFY(immediateResult);
        QVERIFY(delayedResult);
    }

    // TODO: Test timeout
    static void testWaitFor() { XUtils::waitFor(timer()); }

    // TODO: Test timeout
    static void testWaitForWithValue() {
            auto const result { XUtils::waitFor([]() -> XUtils::XCoroTask<int> {
                co_await timer();
                co_return 42;
            }())
        };
        QCOMPARE(result, 42);
    }

    static void testEarlyReturnWaitFor()
    { XUtils::waitFor([]() -> XUtils::XCoroTask<> { co_return; }()); }

    static void testEarlyReturnWaitForWithValue() {
        auto const result{
            XUtils::waitFor([]() -> XUtils::XCoroTask<int> {
                co_return 42;
            }())
        };
        QCOMPARE(result, 42);
    }

    static void testWaitForAwaitable() {
        TestAwaitable awaitable { 42 };
        QElapsedTimer timer;
        timer.start();

        static_assert(std::is_same_v<decltype(XUtils::waitFor(awaitable)), int>);
        auto const result{ XUtils::waitFor(awaitable) };
        QCOMPARE(result, 42);
        QVERIFY(timer.elapsed() >= static_cast<float>(awaitable.delay().count()) * 0.9);
    }

    static void testWaitForVoidAwaitable() {
        TestAwaitable<void> awaitable{};
        QElapsedTimer timer{};
        timer.start();
        static_assert(std::is_void_v<decltype(XUtils::waitFor(awaitable))>);
        XUtils::waitFor(awaitable);
        QVERIFY(timer.elapsed() >= static_cast<float>(awaitable.delay().count()) * 0.9);
    }

    static void testWaitForAwaitableWithOperatorCoAwait() {
        TestAwaitableWithCoAwait awaitable {42};
        XUtils::waitFor(awaitable);
        QElapsedTimer timer;
        timer.start();

        static_assert(std::is_same_v<decltype(XUtils::waitFor(awaitable)), int>);
        auto const result{ XUtils::waitFor(awaitable) };
        QCOMPARE(result, 42);
        QVERIFY(timer.elapsed() >= (90ms).count());
    }

    static void testWaitForVoidAwaitableWithOperatorCoAwait() {
        TestAwaitableWithCoAwait<> awaitable {};
        QElapsedTimer timer {};
        timer.start();
        static_assert(std::is_void_v<decltype(XUtils::waitFor(awaitable))>);
        XUtils::waitFor(awaitable);
        QVERIFY(timer.elapsed() >= (90ms).count());
    }

    static void testWaitForWithValueRethrowsException() {
        auto constexpr coro { []() -> XUtils::XCoroTask<int> {
                co_await timer();
                throw std::runtime_error("Exception");
                co_return 42;
            }
        };

#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, XUtils::waitFor(coro()));
#else
        QVERIFY_EXCEPTION_THROWN(XUtils::waitFor(coro()), std::runtime_error);
#endif
    }

    void testWaitForRethrowsException() {
        auto constexpr coro {
            []() -> XUtils::XCoroTask<> { co_await timer(); throw std::runtime_error("Exception"); }
        };

#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, XUtils::waitFor(coro()));
#else
        QVERIFY_EXCEPTION_THROWN(XUtils::waitFor(coro()), std::runtime_error);
#endif
    }

    void testIgnoredVoidTaskResult() {
        QEventLoop el{};
        ignoreCoroutineResult(el, [&el]() -> XUtils::XCoroTask<> {
            co_await timer();
            el.quit();
        });
    }

    void testIgnoredValueTaskResult() {
        QEventLoop el{};
        ignoreCoroutineResult(el, [&el]() -> XUtils::XCoroTask<QString> {
            co_await timer(); el.quit();
            co_return QStringLiteral("Result");
        });
    }

    static void testThenVoidNoArgument() {
        QEventLoop el{};
        { timer().then([&el]{ el.quit(); }); }
        el.exec();
    }

    static void testThenDiscardsReturnValue() {
        QEventLoop el{};
        bool called {};
        timerWithValue(42).then([&]{ el.quit(); called = true; });
        el.exec();
        QVERIFY(called);
    }

    static void testThenScope() {
        QEventLoop el{};
        thenScopeTestFunc(&el);
        el.exec();
    }

    static void testThenVoidWithArgument() {
        QEventLoop el{};
        int result {};
        {
            timerWithValue(42).then([&el, &result](int const val){
                result = val;
                el.quit();
            });
        }

        el.exec();
        QCOMPARE(result, 42);
    }

    static void testThenVoidWithFunction() {
        QEventLoop el{};
        timerWithValue(10ms).then(timer).then([&el]{ el.quit(); });
        el.exec();
    }

    static void testThenErrorInCallback() {
        QEventLoop el{};
        QTimer::singleShot(5s, &el, [&el]{ el.quit(); QFAIL("Timeout waiting for coroutine"); });

        []() -> XUtils::XCoroTask<> {
            co_await timer();
        }().then([] {
            throw std::runtime_error("Test!");
        }, [](std::exception const &) {
            QFAIL("Continuation exception should not be handled by the same error handled");
        }).then([]() {
            QFAIL("Second then continuation should not be called.");
        }, [&el](std::exception const &) {
            el.quit();
        });

        el.exec();
    }

    static void testThenExceptionInError() {
        QEventLoop el{};
        QTimer::singleShot(5s, &el, [&el]{el.quit(); QFAIL("Timeout waiting for coroutine"); });

        []() -> XUtils::XCoroTask<> {
            co_await timer();
            throw std::runtime_error("Test!");
        }().then([] {
            QFAIL("The then() continuation should not be called");
        }, [](std::exception const &) {
            throw std::runtime_error("Another test!");
        }).then([] {
            QFAIL("Second then() continuation should not be called");
        }, [&el](std::exception const &) {
            el.quit();
        });

        el.exec();
    }

    static void testTaskConnectContext_coro() {
        auto task { timer(200ms) };
        static_assert(std::is_same_v<decltype(task), XUtils::XCoroTask<>>);

        bool called {};

        {
            auto const context { std::make_unique<QObject>() };

            XUtils::connect(task, context.get(), [&]{ called = true; });
        }

        // Delete context, callback should not be called

        XUtils::waitFor(task);

        QVERIFY(!called);
    }
#endif

};

QTEST_GUILESS_MAIN(QCoroTaskTest)

#include "qcorotask.moc"
