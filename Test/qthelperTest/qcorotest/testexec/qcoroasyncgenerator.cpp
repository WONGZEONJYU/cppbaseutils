#include <XCoroutine/xcoroasyncgenerator.hpp>
#include <XQtHelper/qcoro/network/qcoronetworkreply.hpp>
#include <XQtHelper/qcoro/core/qcorotimer.hpp>
#include <testobject.hpp>
#include <testhttpserver.hpp>
#include <vector>
#include <QScopeGuard>
#include <QHostAddress>
#include <QTcpServer>

struct NoCopyMove {
    Q_IMPLICIT constexpr NoCopyMove(int const val): m_val(val) {}
    Q_DISABLE_COPY_MOVE(NoCopyMove)
    ~NoCopyMove() = default;
    int m_val{};
};

struct MoveOnly {
    Q_IMPLICIT constexpr MoveOnly(int const val): m_val(val) {}
    Q_DISABLE_COPY(MoveOnly)
    X_DEFAULT_MOVE(MoveOnly)
    ~MoveOnly() = default;
    int m_val{};
};

using namespace std::chrono_literals;

XUtils::XCoroTask<> sleep(std::chrono::milliseconds ms) {
    QTimer timer{};
    timer.start(ms);
    co_await timer;
}

struct AsyncGeneratorTest : QCoro::TestObject<AsyncGeneratorTest> {
    Q_OBJECT

    XUtils::XCoroTask<> testGenerator_coro(QCoro::TestContext) {
        auto constexpr createGenerator {
            []() -> XUtils::XAsyncGenerator<int> {
                for (int i {}; i < 10; i++) {
                    QTimer timer{};
                    timer.start(50ms);
                    co_await XUtils::qCoro(timer).waitForTimeout();
                    co_yield i;
                }
            }
        };

        std::vector<int> values{};
        X_CORO_FOREACH(int val, createGenerator())
        { values.push_back(val); }

        QCORO_COMPARE(values, (std::vector{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
    }

    XUtils::XCoroTask<> testSyncGenerator_coro(QCoro::TestContext ctx) {
        ctx.setShouldNotSuspend();

        auto constexpr createGenerator {
            []() -> XUtils::XAsyncGenerator<int> { for (int i {}; i < 10; ++i) { co_yield i; } }
        };

        std::vector<int> values;
        X_CORO_FOREACH(int val, createGenerator())
        { values.push_back(val); }

        QCORO_COMPARE(values, (std::vector{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
    }

    XUtils::XCoroTask<> testTerminateSuspendedGenerator_coro(QCoro::TestContext ctx) {
        ctx.setShouldNotSuspend();

        bool destroyed {};

        {

            auto const createGenerator { [&destroyed]() -> XUtils::XAsyncGenerator<int> {
                    auto const guard { qScopeGuard([&destroyed]{ destroyed = true; }) };
                    auto const pointer { std::make_unique<QString>(
                        QStringLiteral("This should get destroyed. If not, ASAN will catch it.")) };
                    while (true) { co_yield 42; }
                }
            };

            auto const generator { createGenerator() };
            auto const it { co_await generator.begin()};
            QCORO_COMPARE(*it, 42);
        } // The generator gets destroyed here. Everything on generator's stack is destroyed.

        QCORO_VERIFY(destroyed);
    }

    XUtils::XCoroTask<> testEmptyGenerator_coro(QCoro::TestContext ctx) {
        ctx.setShouldNotSuspend();
#undef IF_FALSE
#define IF_FALSE if (false) { co_yield 42; }  // NOLINT(readability-simplify-boolean-expr)
        auto constexpr createGenerator { []() -> XUtils::XAsyncGenerator<int> { IF_FALSE } };
        auto const generator { createGenerator() };
        QCORO_COMPARE(co_await generator.begin(), generator.end());
#undef IF_FALSE
    }

    XUtils::XCoroTask<> testReferenceGenerator_coro(QCoro::TestContext) {
        auto constexpr createGenerator {
            []() -> XUtils::XAsyncGenerator<NoCopyMove &> {
                for (int i {}; i < 8; i += 2) {
                    NoCopyMove val{i};
                    co_await sleep(10ms);
                    co_yield val;
                    QCORO_COMPARE(val.m_val, i + 1);
                }
            }
        };

        int testValue {};
        X_CORO_FOREACH(NoCopyMove &val, createGenerator()) {
            QCORO_COMPARE(val.m_val, testValue);
            ++val.m_val;
            testValue += 2;
        }
        QCORO_COMPARE(testValue, 8);
    }

    XUtils::XCoroTask<> testConstReferenceGenerator_coro(QCoro::TestContext) {
        auto constexpr createGenerator {
            []() -> XUtils::XAsyncGenerator<NoCopyMove const &> {
                for (int i {}; i < 4; ++i) {
                    NoCopyMove const value{i};
                    co_await sleep(10ms);
                    co_yield value;
                }
            }
        };

        int testValue {};
        X_CORO_FOREACH(NoCopyMove const & val, createGenerator())
        { QCORO_COMPARE(val.m_val, testValue++); }
        QCORO_COMPARE(testValue, 4);
    }

    XUtils::XCoroTask<> testMoveonlyGenerator_coro(QCoro::TestContext) {
        auto constexpr createGenerator {
            []() -> XUtils::XAsyncGenerator<MoveOnly> {
                for (int i {}; i < 4; ++i) {
                    MoveOnly value{i};
                    co_await sleep(10ms);
                    co_yield std::move(value);
                }
            }
        };

        auto const generator { createGenerator() };
        int testValue {};
        for (auto it { co_await generator.begin()}, end = generator.end(); it != end; co_await ++it) {
           auto const value{ std::move(*it)};
            QCORO_COMPARE(value.m_val, testValue++);
        }
        QCORO_COMPARE(testValue, 4);
    }

    XUtils::XCoroTask<> testMovedGenerator_coro(QCoro::TestContext) {
        using AsyncGenerator = XUtils::XAsyncGenerator<int>;
        AsyncGenerator generator {};

        {
            auto constexpr createGenerator {
                []() -> AsyncGenerator { for (int i {}; i < 4; ++i)  { co_await sleep(10ms); co_yield i; } }
            };

          auto originalGenerator { createGenerator()};
          generator = std::move(originalGenerator);
        }

        int testValue {};
        for (auto it { co_await generator.begin() }, end = generator.end(); it != end; co_await ++it) {
            auto const value{ *it };
            QCORO_COMPARE(value, testValue++);
        }
        QCORO_COMPARE(testValue, 4);
    }

    XUtils::XCoroTask<> testException_coro(QCoro::TestContext) {
        auto constexpr createGenerator {
            []() -> XUtils::XAsyncGenerator<int> {
                for (int i {}; i < 4; ++i) {
                    co_await sleep(10ms);
                    if (i == 2) { throw std::runtime_error("Two?! I can't handle that much!"); }
                    co_yield i;
                }
            }
        };

        auto const generator { createGenerator() };
        auto it { co_await generator.begin() };
        QCORO_VERIFY(it != generator.end());
        QCORO_COMPARE(*it, 0);
        co_await ++it;
        QCORO_VERIFY(it != generator.end());
        QCORO_COMPARE(*it, 1);
        QCORO_VERIFY_EXCEPTION_THROWN(co_await ++it, std::runtime_error);
        QCORO_COMPARE(it, generator.end());
    }

    XUtils::XCoroTask<> testExceptionInDereference_coro(QCoro::TestContext) {
        auto constexpr createGenerator {
            []() -> XUtils::XAsyncGenerator<int> {
                for (int i {}; i < 4; ++i) {
                    co_await sleep(10ms);
                    if (i == 2) { throw std::runtime_error("I already told you two is too much"); }
                    co_yield i;
                }
            }
        };

        auto const generator { createGenerator() };
        auto it { co_await generator.begin() };
        QCORO_VERIFY(it != generator.end());
        QCORO_COMPARE(*it, 0);
        co_await ++it;
        QCORO_VERIFY(it != generator.end());
        QCORO_COMPARE(*it, 1);
        co_await ++it;
        QCORO_VERIFY_EXCEPTION_THROWN(*it, std::runtime_error);
        QCORO_COMPARE(it, generator.end());
    }

    XUtils::XCoroTask<> testExceptionInBegin_coro(QCoro::TestContext) {

        auto constexpr createGenerator {
            [is_throw = true]() -> XUtils::XAsyncGenerator<uint64_t> {
                co_await sleep(10ms);
                // NOTE: The condition here is a necessary workaround for Clang being too clever,
                // seeing that `co_yield` will never be reached and optimizing it away, thus breaking
                // the coroutine. With this condition (or by wrapping the body into a for-loop) the
                // optimization is disabled (as the co_yield could *theoretically* be reached) and
                // the generator behaves as expected.
                if (is_throw) { throw std::runtime_error("I can't even zero!"); }
                co_yield 42;
            }
        };

        auto const generator { createGenerator() };
        QCORO_VERIFY_EXCEPTION_THROWN(co_await generator.begin(), std::runtime_error);
    }

    XUtils::XCoroTask<> testExceptionInBeginSync_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();
        auto constexpr createGenerator {
            []() -> XUtils::XAsyncGenerator<int> {
                throw std::runtime_error("I can't even zero!");
                co_yield 1;
            }
        };

        auto const generator { createGenerator() };
        QCORO_VERIFY_EXCEPTION_THROWN(co_await generator.begin(), std::runtime_error);
    }

    XUtils::XCoroTask<> testAwaitTransform_coro(QCoro::TestContext) {
        TestHttpServer<QTcpServer> server{};
        server.start(QHostAddress::LocalHost);
        QScopeGuard guard([&server] { server.stop(); });

        const auto createGenerator {
            [&server]() -> XUtils::XAsyncGenerator<QByteArray> {
                QNetworkAccessManager nam{};
                auto const reply { nam.get(QNetworkRequest{QUrl{QStringLiteral("http://127.0.0.1:%1/").arg(server.port())}}) };
                // This wouldn't compile without await_transform() in AsyncGeneratorPromise.
                (void)co_await reply;
                co_yield reply->readAll();
            }
        };

        auto const generator { createGenerator()};
        auto iter { co_await generator.begin()};
        QCORO_VERIFY(iter != generator.end());
        QCORO_COMPARE(*iter, QByteArray("abcdef"));
        QCORO_COMPARE(co_await ++iter, generator.end());
    }

private Q_SLOTS:
    addTest(Generator)
    addTest(SyncGenerator)
    addTest(TerminateSuspendedGenerator)
    addTest(EmptyGenerator)
    addTest(ReferenceGenerator)
    addTest(ConstReferenceGenerator)
    addTest(MoveonlyGenerator)
    addTest(MovedGenerator)
    addTest(Exception)
    addTest(ExceptionInDereference)
    addTest(ExceptionInBegin)
    addTest(ExceptionInBeginSync)
    addTest(AwaitTransform)
};

QTEST_GUILESS_MAIN(AsyncGeneratorTest)

#include "qcoroasyncgenerator.moc"
