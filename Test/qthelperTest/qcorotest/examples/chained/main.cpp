#include <XQtHelper/qcoro/core/qcorotimer.hpp>
#include <QCoreApplication>
#include <QTimer>

using namespace std::chrono_literals;

XUtils::XCoroTask<QString> generateRandomString() {
    std::cout << "GenerateRandomString started" << std::endl;
    QTimer timer{};
    timer.start(1s);
    std::cout << "GenerateRandomString \"generating\"..." << std::endl;
    co_await timer;
    std::cout << "GenerateRandomString finished \"generating\"" << std::endl;

    std::cout << "GenerateRandomString co_returning to caller" << std::endl;
    co_return QStringLiteral("RandomString!");
}

XUtils::XCoroTask<qsizetype> generateRandomNumber() {
    std::cout << "GenerateRandomNumber started" << std::endl;
    std::cout << "GenerateRandomNumber co_awaiting on generateRandomString()" << std::endl;
    auto const string { co_await generateRandomString()};
    std::cout << "GenerateRandomNumber successfully co_awaited on generateRandomString() and "
                 "co_returns result"
              << std::endl;
    co_return string.size();
}

XUtils::XCoroTask<> logRandomNumber() {
    std::cout << "LogRandomNumber started" << std::endl;
    std::cout << "LogRandomNumber co_awaiting on generateRandomNumber()" << std::endl;
    auto const number{ co_await generateRandomNumber()};
    std::cout << "Random number for today is: " << number << std::endl;
    qApp->quit();
}

int main(int argc, char **argv) {
    QCoreApplication app{argc, argv};
    QTimer::singleShot(0, qApp, logRandomNumber);
    return app.exec();
}
