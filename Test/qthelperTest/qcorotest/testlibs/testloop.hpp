#ifndef XUTILS2_TESTLOOP_HPP
#define XUTILS2_TESTLOOP_HPP

#pragma once

#include <QEventLoop>
#include <QTimer>

#define DELAYED(expr) QTimer::singleShot(10ms, [&] { expr; })

class TestLoop : public QObject {
    Q_OBJECT
    QEventLoop m_eventLoop_{};
    QTimer m_timer_{};

public:
    Q_IMPLICIT TestLoop(QObject * = nullptr);
    void exec();
    void quit();
};

#endif
