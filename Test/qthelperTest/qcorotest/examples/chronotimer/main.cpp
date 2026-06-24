#include <XQtHelper/qcoro/core/qcorochronotimer.hpp>
#include <QCoreApplication>
#include <QDateTime>

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)

XUtils::XCoroTask<> runMainTimer() {
    std::cout << "runMainTimer started" << std::endl;
    QChronoTimer timer{};
    using namespace std::chrono_literals;
    timer.setInterval(2s);
    timer.start();

    std::cout << "Waiting for main timer..." << std::endl;
    co_await timer;
    std::cout << "Main timer ticked!" << std::endl;

    qApp->quit();
}

int main(int argc, char **argv) {
    QCoreApplication app{argc, argv};
    QChronoTimer ticker{};
    ticker.callOnTimeout(std::addressof(app),[]{
        std::cout << QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString()
                  << " Secondary timer tick!" << std::endl;
    });
    using namespace std::chrono_literals;
    ticker.setInterval(200ms);
    ticker.start();
    QTimer::singleShot(0, runMainTimer);
    return app.exec();
}

#else

int main([[maybe_unused]]int argc, [[maybe_unused]]char *argv[]) {
    std::cout << R"(Not Support QChronoTimer!)" << std::endl;
    return 0;
}

#endif
