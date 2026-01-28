#include <testobject.hpp>
#include <XQtHelper/qcoro/core/qcorotimer.hpp>
#include <XQtHelper/qcoro/core/waitfor.hpp>

struct QCoroWaitForTest : QCoro::TestObject<QCoroWaitForTest> {
    Q_OBJECT

    XUtils::XCoroTask<> testPrimitiveType_coro(QCoro::TestContext ctx) {
        ctx.setShouldNotSuspend();
        auto constexpr task_test { []() -> XUtils::XCoroTask<int> { co_return 7;} };
        auto const ret{ XUtils::waitFor(task_test()) };
        QCORO_VERIFY(ret == 7);
    }

    XUtils::XCoroTask<> testDefaultConstructible_coro(QCoro::TestContext ctx) {
        ctx.setShouldNotSuspend();
        auto constexpr task_test { []() -> XUtils::XCoroTask<std::string> { co_return "seven"; } };
        auto const ret{ XUtils::waitFor(task_test())};
        QCORO_VERIFY(ret == "seven");
    }

    XUtils::XCoroTask<> testNonDefaultConstructible_coro(QCoro::TestContext ctx) {
        ctx.setShouldNotSuspend();

        struct test_struct {
            Q_IMPLICIT constexpr test_struct(int const i) : m_i{i} {}
            int m_i{};
        };

        auto constexpr task_test { []() -> XUtils::XCoroTask<test_struct> {
            co_return test_struct(7);
        }};

        auto const ret{ XUtils::waitFor(task_test()) };
        QCORO_VERIFY(ret.m_i == 7);
    }

private Q_SLOTS:
    addTest(PrimitiveType)
    addTest(DefaultConstructible)
    addTest(NonDefaultConstructible)
};

QTEST_GUILESS_MAIN(QCoroWaitForTest)

#include "qcorowaitfor.moc"
