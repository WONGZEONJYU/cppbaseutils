#include <testobject.hpp>
#include <XQtHelper/qcoro/core/qcorotimer.hpp>
#include <XQtHelper/qcoro/core/qcorosignal.hpp>
#include <QTimer>
#include <QThread>

struct SignalTest : QObject {
    Q_OBJECT
public:
    explicit(false) SignalTest(bool const active = true) {
        if (active) {
            using namespace std::chrono_literals;
            QTimer::singleShot(100ms, this, &SignalTest::emit);
        }
    }

    void emit() {
        Q_EMIT voidSignal();
        Q_EMIT singleArg(QStringLiteral("YAY!"));
        Q_EMIT multiArg(QStringLiteral("YAY!"), 42, this);
        Q_EMIT privateVoid(QPrivateSignal{});
        Q_EMIT privateSingleArg(QStringLiteral("YAY!"), QPrivateSignal{});
        Q_EMIT privateMultiArg(QStringLiteral("YAY!"), 42, this, QPrivateSignal{});
    }

Q_SIGNALS:
    void voidSignal();
    void singleArg(QString const &);
    void multiArg(QString const &, int, QObject *);
    void privateVoid(QPrivateSignal);
    void privateSingleArg(QString const &, QPrivateSignal);
    void privateMultiArg(QString const &, int, QObject *, QPrivateSignal);
    void signalThatsNeverEmitted();
};

struct MultiSignalTest : SignalTest {
    Q_OBJECT
    QTimer m_timer_{};
public:
    explicit MultiSignalTest(bool const active = true)
        : SignalTest {false}
    {
        if (active) {
            using namespace std::chrono_literals;
            m_timer_.setInterval(10ms);
            m_timer_.callOnTimeout(this,&MultiSignalTest::emit);
            m_timer_.start();
        }
    }
};

struct SimpleSignal : QObject {
    Q_OBJECT
public:
    Q_SIGNAL void messageReceived(int);

    void send(int const id) { Q_EMIT messageReceived(id); }

    XUtils::XCoroTask<int> waitForMessage(int const id) {
        X_CORO_FOREACH(int msgId, XUtils::qCoroSignalListener(this, &SimpleSignal::messageReceived))
        { if (msgId == id) { co_return id; } }
        co_return -1;
    }
};

struct QCoroSignalTest : QCoro::TestObject<QCoroSignalTest> {
    Q_OBJECT

    XUtils::XCoroTask<> testTriggers_coro(QCoro::TestContext) {
        SignalTest obj {};
        co_await XUtils::qCoro(std::addressof(obj), &SignalTest::voidSignal);
        static_assert(std::is_same_v<decltype(XUtils::qCoro(&obj, &SignalTest::voidSignal)),XUtils::XCoroTask<std::tuple<>>>);
    }

    XUtils::XCoroTask<> testReturnsValue_coro(QCoro::TestContext) {
        SignalTest obj{};
        auto const result { co_await XUtils::qCoro(std::addressof(obj), &SignalTest::singleArg) };
        static_assert(std::is_same_v<decltype(result), const QString>);
        QCORO_COMPARE(result, QStringLiteral("YAY!"));
    }

    XUtils::XCoroTask<> testReturnsTuple_coro(QCoro::TestContext) {
        SignalTest obj{};
        auto const result { co_await XUtils::qCoro(std::addressof(obj), &SignalTest::multiArg) };
        static_assert(std::is_same_v<decltype(result), const std::tuple<QString, int, QObject *>>);
        auto const [value, number, ptr] { result };
        QCORO_COMPARE(value, QStringLiteral("YAY!"));
        QCORO_COMPARE(number, 42);
        QCORO_COMPARE(ptr, &obj);
    }

    XUtils::XCoroTask<> testTimeoutTriggersVoid_coro(QCoro::TestContext) {
        SignalTest obj {};
        using namespace std::chrono_literals;
        auto const result{ co_await XUtils::qCoro(std::addressof(obj), &SignalTest::voidSignal, 10ms) };
        static_assert(std::is_same_v<decltype(result), const std::optional<std::tuple<>>>);
        QCORO_VERIFY(!result.has_value());
    }

    XUtils::XCoroTask<> testTimeoutVoid_coro(QCoro::TestContext) {
        SignalTest obj{};
        using namespace std::chrono_literals;
        auto const result{ co_await XUtils::qCoro(std::addressof(obj), &SignalTest::voidSignal, 1s) };
        static_assert(std::is_same_v<decltype(result), const std::optional<std::tuple<>>>);
        QCORO_VERIFY(result.has_value());
    }

    XUtils::XCoroTask<> testTimeoutTriggersValue_coro(QCoro::TestContext) {
        SignalTest obj{};
        using namespace std::chrono_literals;
        const auto result = co_await XUtils::qCoro(std::addressof(obj), &SignalTest::singleArg, 10ms);
        static_assert(std::is_same_v<decltype(result), const std::optional<QString>>);
        QCORO_VERIFY(!result.has_value());
    }

    XUtils::XCoroTask<> testTimeoutValue_coro(QCoro::TestContext) {
        SignalTest obj;
        using namespace std::chrono_literals;
        auto const result = co_await XUtils::qCoro(std::addressof(obj), &SignalTest::singleArg, 1s);
        static_assert(std::is_same_v<decltype(result), const std::optional<QString>>);
        QCORO_VERIFY(result.has_value());
        QCORO_COMPARE(*result, QStringLiteral("YAY!"));
    }

    XUtils::XCoroTask<> testTimeoutTriggersTuple_coro(QCoro::TestContext) {
        SignalTest obj;
        using namespace std::chrono_literals;
        auto const result { co_await XUtils::qCoro(std::addressof(obj), &SignalTest::multiArg, 10ms) };
        static_assert(std::is_same_v<
                decltype(result),
                const std::optional<std::tuple<QString, int, QObject *>>>);
        QCORO_VERIFY(!result.has_value());
    }

    XUtils::XCoroTask<> testTimeoutTuple_coro(QCoro::TestContext) {
        SignalTest obj{};
        using namespace std::chrono_literals;
        auto const result { co_await XUtils::qCoro(std::addressof(obj), &SignalTest::multiArg, 1s) };
        static_assert(std::is_same_v<
                decltype(result),
                const std::optional<std::tuple<QString, int, QObject *>>>);
        QCORO_VERIFY(result.has_value());
        QCORO_COMPARE(std::get<0>(*result), QStringLiteral("YAY!"));
        QCORO_COMPARE(std::get<1>(*result), 42);
        QCORO_COMPARE(std::get<2>(*result), &obj);
    }

    static void testThenTriggers_coro(TestLoop & el) {
        SignalTest obj{};
        bool triggered {};
        auto const f { [&el,&triggered]{ triggered = true; el.quit(); } };
        XUtils::qCoro(std::addressof(obj), &SignalTest::voidSignal).then(f);
        el.exec();
        QVERIFY(triggered);
    }

    static void testThenReturnsValue_coro(TestLoop & el) {
        SignalTest obj{};
        std::optional<QString> value {};
        auto const f { [&value,&el](QString const & arg){ value = arg; el.quit();} };
        XUtils::qCoro(std::addressof(obj), &SignalTest::singleArg).then(f);
        el.exec();
        QVERIFY(value.has_value());
        QCOMPARE(*value, QStringLiteral("YAY!"));
    }

    static void testThenReturnsTuple_coro(TestLoop &el) {
        SignalTest obj{};
        std::optional<QString> str{};
        std::optional<int> num{};
        std::optional<QObject *> ptr{};

        auto const f { [&el,&str,&num,&ptr](std::tuple<QString, int, QObject *> const & args) {
            str = std::get<0>(args);
            num = std::get<1>(args);
            ptr = std::get<2>(args);
            el.quit();
        } };

        XUtils::qCoro(std::addressof(obj), &SignalTest::multiArg).then(f);
        el.exec();

        QVERIFY(str.has_value());
        QVERIFY(num.has_value());
        QVERIFY(ptr.has_value());
        QCOMPARE(*str, QStringLiteral("YAY!"));
        QCOMPARE(*num, 42);
        QCOMPARE(*ptr, &obj);
    }

    XUtils::XCoroTask<> testThenChained_coro(QCoro::TestContext) {
        SignalTest obj{};
        auto constexpr f { []<typename Tp>(Tp && arg) -> XUtils::XCoroTask<QString> {
                QTimer timer{};
                using namespace std::chrono_literals;
                timer.start(100ms);
                co_await timer;
                co_return std::forward<Tp>(arg) + std::forward<Tp>(arg);
            }
        };
        auto const result { co_await XUtils::qCoro(std::addressof(obj), &SignalTest::singleArg).then(f) };
        QCORO_COMPARE(result, QStringLiteral("YAY!YAY!"));
    }

    XUtils::XCoroTask<> testVoidQPrivateSignal_coro(QCoro::TestContext) {
        SignalTest obj{};
        auto const result { co_await XUtils::qCoro(std::addressof(obj), &SignalTest::privateVoid) };
        static_assert(std::is_same_v< decltype(result), const std::tuple<> >);
        Q_UNUSED(result);
    }

    XUtils::XCoroTask<> testSingleArgQPrivateSignal_coro(QCoro::TestContext) {
        SignalTest obj{};
        auto const result { co_await XUtils::qCoro(std::addressof(obj), &SignalTest::privateSingleArg) };
        static_assert(std::is_same_v<decltype(result), const QString>);
        QCORO_COMPARE(result, QStringLiteral("YAY!"));
    }

    XUtils::XCoroTask<> testMultiArgQPrivateSignal_coro(QCoro::TestContext) {
        SignalTest obj{};
        auto const [str, num, ptr] { co_await XUtils::qCoro(std::addressof(obj), &SignalTest::privateMultiArg) };
        static_assert(std::is_same_v<decltype(str), const QString>);
        static_assert(std::is_same_v<decltype(num), const int>);
        static_assert(std::is_same_v<decltype(ptr), QObject * const>);
        QCORO_COMPARE(str, QStringLiteral("YAY!"));
        QCORO_COMPARE(num, 42);
        QCORO_COMPARE(ptr, &obj);
    }

    XUtils::XCoroTask<> testSignalListenerVoid_coro(QCoro::TestContext) {
        MultiSignalTest obj{};
        auto generator { XUtils::qCoroSignalListener(std::addressof(obj), &MultiSignalTest::voidSignal) };
        int count = 0;
        X_CORO_FOREACH(const std::tuple<> & value, generator)
        { Q_UNUSED(value); if (++count == 10) { break; } }

        QCORO_COMPARE(count, 10);
    }

    XUtils::XCoroTask<> testSignalListenerValue_coro(QCoro::TestContext) {
        MultiSignalTest obj{};
        auto generator { XUtils::qCoroSignalListener(std::addressof(obj), &MultiSignalTest::singleArg) };
        int count {};
        X_CORO_FOREACH(auto && value, generator) {
            QCORO_COMPARE(value, QStringLiteral("YAY!"));
            if (++count == 10) { break; }
        }

        QCORO_COMPARE(count, 10);
    }

    XUtils::XCoroTask<> testSignalListenerTuple_coro(QCoro::TestContext) {
        MultiSignalTest obj {};

        auto generator { XUtils::qCoroSignalListener(std::addressof(obj), &MultiSignalTest::multiArg)};
        int count {};
        X_CORO_FOREACH( auto && value, generator) {
            QCORO_COMPARE(std::get<0>(value), QStringLiteral("YAY!"));
            QCORO_COMPARE(std::get<1>(value), 42);
            QCORO_COMPARE(std::get<2>(value), std::addressof(obj));
            if (++count == 10) { break; }
        }

        QCORO_COMPARE(count, 10);
    }

    XUtils::XCoroTask<> testSignalListenerTimeout_coro(QCoro::TestContext) {
        QObject obj{};
        // A signal that doesn't get invoked
        using namespace std::chrono_literals;
        auto generator { XUtils::qCoroSignalListener(std::addressof(obj), &QObject::destroyed, 1ms) };
        X_CORO_FOREACH(auto && value, generator) {
            Q_UNUSED(value);
            QCORO_FAIL("The signal should time out and the generator should not return invalid iterator.");
        }
    }

    XUtils::XCoroTask<> testSignalListenerQueue_coro(QCoro::TestContext ctx) {
        SignalTest test {{}};
        // I have a generator
        auto generator { XUtils::qCoroSignalListener(std::addressof(test), &SignalTest::voidSignal) };
        // I emit signals that the generator is listening to, the generator
        // should enqueue them.
        for (int i {}; i < 10; ++i) { test.emit(); }

        // I asynchronously wait for first iterator
        auto it { co_await generator.begin()};
        int count {};
        ctx.setShouldNotSuspend();
        // I loop over generator - this should not suspend as we are simply consuming
        // events from the queue.
        for (; it != generator.end(); co_await ++it)
        { if (++count == 10) { break; } }
        QCORO_COMPARE(count, 10);
    }

    XUtils::XCoroTask<> testSignalAfterListenerQuits_coro(QCoro::TestContext) {
        SimpleSignal simple {};
        auto msg1{ simple.waitForMessage(1)}
            ,msg2{ simple.waitForMessage(2)};
        simple.send(1);
        simple.send(2);
        QCORO_COMPARE(co_await msg1, 1);
        QCORO_COMPARE(co_await msg2, 2);
    }

    XUtils::XCoroTask<> testSignalListenerQPrivateSignalVoid_coro(QCoro::TestContext) {
        MultiSignalTest obj{};

        auto generator { XUtils::qCoroSignalListener(std::addressof(obj), &MultiSignalTest::privateVoid) };
        int count {};
        X_CORO_FOREACH(auto const & value, generator) {
            static_assert(std::is_same_v<decltype(value), const std::tuple<> &>);
            Q_UNUSED(value);
            if (++count == 10) { break; }
        }

        QCORO_COMPARE(count, 10);
    }

    XUtils::XCoroTask<> testSignalListenerQPrivateSignalValue_coro(QCoro::TestContext) {
        MultiSignalTest obj{};

        auto generator { XUtils::qCoroSignalListener(std::addressof(obj), &MultiSignalTest::privateSingleArg) };
        int count {};
        X_CORO_FOREACH(auto const & value, generator) {
            static_assert(std::is_same_v<decltype(value), const QString &>);
            QCORO_COMPARE(value, QStringLiteral("YAY!"));
            if (++count == 10) { break; }
        }

        QCORO_COMPARE(count, 10);
    }

    XUtils::XCoroTask<> testSignalListenerQPrivateSignalTuple_coro(QCoro::TestContext) {
        MultiSignalTest obj {};

        auto generator { XUtils::qCoroSignalListener(std::addressof(obj), &MultiSignalTest::privateMultiArg) };
        int count {};
        X_CORO_FOREACH(auto const & value, generator) {
            static_assert(std::is_same_v<decltype(value), const std::tuple<QString, int, QObject *> &>);
            QCORO_COMPARE(std::get<QString>(value), QStringLiteral("YAY!"));
            QCORO_COMPARE(std::get<int>(value), 42);
            QCORO_COMPARE(std::get<QObject *>(value), &obj);
            if (++count == 10) { break; }
        }

        QCORO_COMPARE(count, 10);
    }

    XUtils::XCoroTask<> testSignalEmitterOnDifferentThread_coro(QCoro::TestContext) {
        SignalTest test{};
        QThread thread {};
        test.moveToThread(&thread);
        thread.start();
        co_await XUtils::qCoro(std::addressof(test), &SignalTest::voidSignal);
        // Make sure we are resumed on our thread
        QCORO_COMPARE(QThread::currentThread(), qApp->thread());
        using namespace std::chrono_literals;
        co_await XUtils::qCoro(std::addressof(test), &SignalTest::signalThatsNeverEmitted, 20ms);
        // Make sure we are resumed on our thread after the timeout
        QCORO_COMPARE(QThread::currentThread(), qApp->thread());

        thread.quit();
        thread.wait();
    }

private Q_SLOTS:
    addTest(Triggers)
    addTest(ReturnsValue)
    addTest(ReturnsTuple)
    addTest(TimeoutVoid)
    addTest(TimeoutTriggersVoid)
    addTest(TimeoutValue)
    addTest(TimeoutTriggersValue)
    addTest(TimeoutTuple)
    addTest(TimeoutTriggersTuple)
    addTest(ThenChained)
    addTest(VoidQPrivateSignal)
    addTest(SingleArgQPrivateSignal)
    addTest(MultiArgQPrivateSignal)
    addThenTest(Triggers)
    addThenTest(ReturnsValue)
    addThenTest(ReturnsTuple)
    addTest(SignalListenerVoid)
    addTest(SignalListenerValue)
    addTest(SignalListenerTuple)
    addTest(SignalListenerTimeout)
    addTest(SignalListenerQueue)
    addTest(SignalAfterListenerQuits)
    addTest(SignalListenerQPrivateSignalVoid)
    addTest(SignalListenerQPrivateSignalValue)
    addTest(SignalListenerQPrivateSignalTuple)
    addTest(SignalEmitterOnDifferentThread)
};

QTEST_GUILESS_MAIN(QCoroSignalTest)

#include "qcorosignal.moc"
