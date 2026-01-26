#include <XCoroutine/xcorogenerator.hpp>
#include <XQtHelper/qcoro/test/qcorotest.hpp>
#include <QObject>
#include <QTest>
#include <QScopeGuard>

struct NoCopyMove {
    int m_val;
    explicit constexpr NoCopyMove(int const val): m_val{val} {}
    Q_DISABLE_COPY_MOVE(NoCopyMove)
    ~NoCopyMove() = default;
};

struct MoveOnly {
    int m_val;
    explicit constexpr MoveOnly(int const val): m_val{val} {}
    Q_DISABLE_COPY(MoveOnly)
    X_DEFAULT_MOVE(MoveOnly)
    ~MoveOnly() = default;

};

struct GeneratorTest : QObject {
    Q_OBJECT

private Q_SLOTS:

    static void testImmediateGenerator() {
        auto constexpr createGenerator { []() -> XUtils::XGenerator<int>
        { for (int value {}; value < 10; ++value)  { co_yield value; } }};

        auto const generator { createGenerator()};
        std::vector<int> values{};
        for (auto it {generator.begin()}, end = decltype(generator)::end(); it != end; ++it) {
            values.emplace_back(*it);
        }

        QCOMPARE(values.size(), 10U);
        QCOMPARE(values, (std::vector{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
    }

    static void testTerminateSuspendedGenerator() {
        bool destroyed {};

        {
            auto const createGenerator { [&destroyed]() -> XUtils::XGenerator<int> {
                    auto const guard { qScopeGuard([&destroyed]{ destroyed = true; })};
                    auto const pointer { std::make_unique<QString>(QStringLiteral("This should get destroyed. If not, ASAN will catch it."))};
                    while (true) { co_yield 42; }
                }
            };

            auto const generator { createGenerator()};
            auto const it { generator.begin()};
            QCOMPARE(*it, 42);
        } // The generator gets destroyed here.

        QVERIFY(destroyed);
    }

    void testEmptyGenerator() {
        auto constexpr createGenerator { []() -> XUtils::XGenerator<int> {
            if (false) { // NOLINT(readability-simplify-boolean-expr)
                co_yield 42; // Make it a coroutine, except it never gets invoked.
            }
        }};

        auto const generator { createGenerator()};
        auto const begin { generator.begin() };
        QCOMPARE(begin, generator.end());
    }

    static void testConstReferenceGenerator() {
        auto constexpr createGenerator { []() -> XUtils::XGenerator<NoCopyMove const &>
            { for (int i {}; i < 4; ++i) { NoCopyMove const val { i }; co_yield val; } }
        };

        auto const generator { createGenerator()};
        int testVal {};
        for (auto it { generator.begin() };it != decltype(generator)::end(); ++it) {
            auto && value{ *it};
            QCOMPARE(value.m_val, testVal++);
        }
        QCOMPARE(testVal, 4);
    }

    static void testReferenceGenerator() {
        auto constexpr createGenerator { []() -> XUtils::XGenerator<NoCopyMove &> {
                for (int i {}; i < 8; i += 2) {
                    NoCopyMove val(i); co_yield val;
                    QCORO_COMPARE(val.m_val, i + 1);
                }
            }
        };

        auto const generator { createGenerator()};
        int testVal {};
        for (auto it = generator.begin();it != decltype(generator)::end();++it) {
            auto && value{ *it };
            QCOMPARE(value.m_val, testVal);
            value.m_val += 1;
            testVal += 2;
        }
    }

    static void testMoveonlyGenerator() {
        auto constexpr createGenerator { []() -> XUtils::XGenerator<MoveOnly &&>
            { for (int i {}; i < 4; ++i) { co_yield MoveOnly{i}; } }
        };

        auto const generator { createGenerator() };

        int testVal {};
        for (auto it { generator.begin() };it != decltype(generator)::end(); ++it) {
            auto const val{ std::move(*it)};
            QCOMPARE(val.m_val, testVal++);
        }
        QCOMPARE(testVal, 4);
    }

    static void testMovedGenerator() {
        auto constexpr createGenerator {
            []() -> XUtils::XGenerator<int> { for (int i {}; i < 4; ++i) { co_yield i;} }
        };

        auto originalGenerator { createGenerator() };
        auto const generator { std::move(originalGenerator) };

        int testVal {};
        for (auto it = generator.begin();it != decltype(generator)::end();++it) {
            auto const value{ *it };
            QCOMPARE(value, testVal++);
        }
        QCOMPARE(testVal, 4);
    }

    static void testException(){

        auto constexpr createGenerator {
            []() -> XUtils::XGenerator<int> {
                for (int i {}; i < 10; ++i) {
                    if (2 == i) { throw std::runtime_error("Two?! I can't handle two!!"); }
                    co_yield i;
                }
            }
        };

        auto const generator { createGenerator() };
        auto it { generator.begin()};
        QVERIFY(it != decltype(generator)::end());
        QCOMPARE(*it, 0);
        ++it;
        QVERIFY(it != generator.end());
        QCOMPARE(*it, 1);

#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, ++it);
#else
        QVERIFY_EXCEPTION_THROWN(++it, std::runtime_error);
#endif
        QCOMPARE(it, generator.end());
    }

    static void testExceptionInBegin() {
        auto const generator { []() -> XUtils::XGenerator<int> {
            throw std::runtime_error("Zero is too small!");
            co_yield 1;
        }()};

#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, generator.begin());
#else
        QVERIFY_EXCEPTION_THROWN(generator.begin(), std::runtime_error);
#endif
    }
};

QTEST_GUILESS_MAIN(GeneratorTest)

#include "qcorogenerator.moc"
