#include <XQtHelper/qcoro/core/qcoroiodevice.hpp>

#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

using namespace std::chrono_literals;

struct Server : QObject {
    Q_OBJECT
    QTcpServer m_server_{};

public:
    Q_IMPLICIT Server(QHostAddress const & addr, uint16_t const port) {
        m_server_.listen(addr, port);
        connect(std::addressof(m_server_),
            &QTcpServer::newConnection, this, &Server::handleConnection);
    }

private Q_SLOTS:
    XUtils::XCoroTask<> handleConnection() {
        auto const socket { m_server_.nextPendingConnection()};
        while (socket->isOpen()) {
            auto const data{ co_await socket };
            socket->write("PONG: " + data);
        }
    }
};

struct Client : QObject {
    Q_OBJECT
    int m_ping_ {};
    QTcpSocket m_socket_{};
    QTimer m_timer_ {};

public:
    Q_IMPLICIT Client(QHostAddress const & addr, uint16_t const port) {
        m_socket_.connectToHost(addr, port);
        m_timer_.callOnTimeout(this, &Client::sendPing);
        m_timer_.start(300ms);
    }

private Q_SLOTS:
    XUtils::XCoroTask<> sendPing() {
        std::cout << "Sending ping..." << std::endl;
        m_socket_.write(QByteArray("PING #") + QByteArray::number(++m_ping_));
        auto const response{ co_await m_socket_ };
        std::cout << "Received pong: " << response.constData() << std::endl;
    }
};

int main(int argc, char **argv) {
    QCoreApplication app {argc, argv};
    Server server{QHostAddress::LocalHost, 6666};
    Client client{QHostAddress::LocalHost, 6666};
    return app.exec();
}

#include "main.moc"
