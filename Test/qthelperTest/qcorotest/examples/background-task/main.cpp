#include <XQtHelper/qcoro/core/qcorotimer.hpp>
#include <XQtHelper/qcoro/core/qcoroiodevice.hpp>
#include <XQtHelper/qcoro/core/waitfor.hpp>
#include <QCoreApplication>
#include <QFile>
#include <QDebug>

using namespace std::chrono_literals;

// We could use std::stop_token, but it is not supported by AppleClang
class Stop {
    bool m_shouldStop_ {};
public:
    constexpr void requestStop() noexcept { m_shouldStop_ = true; }
    constexpr bool stopRequested() const noexcept { return m_shouldStop_; }
};

XUtils::XCoroTask<> backgroundTask(Stop const & stop) {
    qDebug() << "Task: Background task started, waiting for event loop";
    co_await XUtils::sleepFor(0ms);
    qDebug() << "Task: Event loop is running";
    QFile file { QStringLiteral("/dev/stdin") };
    (void)file.open(QIODevice::ReadOnly | QIODevice::Unbuffered);
    while (!stop.stopRequested()) {
        qDebug() << "Task: Waiting for input...";
        if ( auto const result{ co_await XUtils::qCoro(file).readLine(1024, 5s) };
            !result.isEmpty())
        { qDebug() << "Task: Read line:" << result; }
        else { qDebug() << "Task: Timeout!"; }
    }
    qDebug() << "Task: Backround task stopped";
}

int main(int argc, char **argv)
{
    QCoreApplication const app{argc, argv};

    Stop stop{};
    auto bgTask { backgroundTask(stop) };
    QObject::connect(std::addressof(app),&QCoreApplication::aboutToQuit, [&stop] {
        qDebug() << "App: Requesting background task to stop";
        stop.requestStop();
    });

    QTimer::singleShot(500ms, std::addressof(app), [] {
        qDebug() << "App: Stopping application";
        QCoreApplication::quit();
    });

    // Run the app
    qDebug() << "App: Starting application event loop";
    auto const result{ app.exec() };
    qDebug() << "App: Application event loop stopped";

    // Wait for the background task to complete
    qDebug() << "App: Waiting for background task to complete";
    XUtils::waitFor(bgTask);
    qDebug() << "App: Background task completed";

    return result;
}
