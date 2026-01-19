#include <testloop.hpp>
#include <QTest>

TestLoop::TestLoop(QObject *parent) : QObject { parent } {
    m_timer_.setSingleShot(true);
    m_timer_.callOnTimeout(this,[this]{ m_eventLoop_.quit(); QFAIL("Test timeout!"); });
}

void TestLoop::exec()
{ m_eventLoop_.exec(); }

void TestLoop::quit() {
    m_timer_.stop();
    QTimer::singleShot(0, std::addressof(m_eventLoop_), &QEventLoop::quit);
}
