#include <XQtHelper/qcoro/core/qcorotimer.hpp>
#include <QCoreApplication>
#include <QDateTime>
#include <QTimer>

using namespace std::chrono_literals;

XUtils::XCoroTask<> runMainTimer() {
    std::cout << "runMainTimer started" << std::endl;
    QTimer timer{};
    timer.setInterval(2s);
    timer.start();

    std::cout << "Waiting for main timer..." << std::endl;
    co_await timer;
    std::cout << "Main timer ticked!" << std::endl;

    qApp->quit();
}

int main(int argc, char **argv) {
    QCoreApplication app{argc, argv};
    QTimer ticker{};
    ticker.callOnTimeout(std::addressof(app),[]{
        std::cout << QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString()
                  << " Secondary timer tick!" << std::endl;
    });
    ticker.start(200ms);
    QTimer::singleShot(0, runMainTimer);
    return app.exec();
}
