#ifndef XUTILS2_TESTOBJECT_HPP
#define XUTILS2_TESTOBJECT_HPP 1

#pragma once

#include <XCoroutine/xcoroutinetask.hpp>
#include <XQtHelper/qcoro/test/qcorotest.hpp>
#include <chrono>
#include <QTest>
#include <QVariant>
#include <testloop.hpp>

namespace QCoro {

class TestContext {
    Q_DISABLE_COPY(TestContext)
    QEventLoop * m_eventLoop_ {};

public:
    explicit (false) TestContext(QEventLoop & el);
    TestContext(TestContext &&) noexcept;
    TestContext &operator=(TestContext &&) noexcept;
    ~TestContext();
    void setShouldNotSuspend() const;
};

struct EventLoopChecker : QTimer {
    Q_OBJECT
    int m_tick_ {},m_minTicks_ {10};

public:
    using milliseconds = std::chrono::milliseconds;

    explicit(false) EventLoopChecker(int const minTicks = 10,milliseconds const interval = milliseconds{5})
        : m_minTicks_{minTicks}
    {
        callOnTimeout(this, [this]{ ++m_tick_; });
        setInterval(interval);
        start();
    }

    explicit(false) operator bool() const noexcept {
        if (m_tick_ < m_minTicks_) { qDebug() << "EventLoopChecker failed: ticks=" << m_tick_ << ", minTicks=" << m_minTicks_; }
        return m_tick_ >= m_minTicks_;
    }
};

template<typename TestClass>
struct TestObject : QObject {

protected:
    explicit(false) TestObject(QObject *parent = {}): QObject{parent}
    {   }

    using testFunction_t = XUtils::XCoroTask<> (TestClass::*)(TestContext);

    void coroWrapper(testFunction_t const testFunction) {
        QEventLoop el{};
        {
            using namespace std::chrono_literals;
            QTimer::singleShot(5s,std::addressof(el),[&el]{ el.exit(1); });
        }

        //(static_cast<TestClass *>(this)->*testFunction)(el);
        std::invoke(testFunction,static_cast<TestClass *>(this),el);

        auto testFinished{ el.property("testFinished").toBool() };
        auto const shouldNotSuspend { el.property("shouldNotSuspend").toBool() };
        if (testFinished) {
            QVERIFY(shouldNotSuspend);
        } else {
            QVERIFY(!shouldNotSuspend);
            auto const result{ el.exec() };
            QVERIFY2(result == 0, "Test function has timed out");
            testFinished = el.property("testFinished").toBool();
            QVERIFY(testFinished);
        }
    }
};

#define addTest(name) void test##name() { coroWrapper(&std::remove_cvref_t<decltype(*this)>::test##name##_coro); }

#define addThenTest(name)  void testThen##name() {  TestLoop loop;  testThen##name##_coro(loop); }

#define addCoroAndThenTests(name)  addTest(name)  addThenTest(name)

}

#endif
