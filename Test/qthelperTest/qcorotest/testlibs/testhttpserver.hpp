#pragma once

#include <QDebug>
#include <QThread>
#include <QTest>

#include <condition_variable>
#include <mutex>
#include <thread>
#include <XAtomic/xatomic.hpp>

class QTcpServer;
class QTcpSocket;
class QLocalServer;
class QLocalSocket;

template<typename Func>
class Thread : public QThread {
    Func m_func_ {};
public:
    explicit Thread(Func && f) : m_func_ {std::forward<Func>(f)} {   }
    ~Thread() override = default;
    void run() override { m_func_(); }
};

template<typename Func>
Thread(Func &&) -> Thread<Func>;

template<typename _>
struct socket_for_server {};

template<> struct socket_for_server<QTcpServer>
{ using type = QTcpSocket; };

template<> struct socket_for_server<QLocalServer>
{ using type = QLocalSocket; };

template<typename ServerType>
class TestHttpServer {
    using SocketType = socket_for_server<ServerType>::type;

    std::unique_ptr<QThread> m_thread_{};
    std::mutex m_readyMutex_ {};
    std::condition_variable m_serverReady_{};
    uint16_t m_port_ {};
    bool m_hasConnection_ {};
    XUtils::XAtomicBool m_stop_ {},m_expectTimeout_ {};

public:

    template<typename T>
    void start(T const & name) {
        m_port_ = 0;
        m_hasConnection_ = {};
        m_stop_.storeRelease({});
        m_expectTimeout_.storeRelease({});
        // Can't use QThread::create, it's only available when Qt is built with C++17,
        // which some distros don't have :(
        m_thread_.reset(new Thread([this, name] { run(name); }));
        m_thread_->start();
        std::unique_lock lock {m_readyMutex_};
        m_serverReady_.wait(lock, [this] { return 0 != m_port_; });
    }

    void stop() {
        m_stop_.storeRelease(true);
        if (m_thread_->isRunning()) { m_thread_->wait(); }
        m_thread_.reset();
        m_port_ = 0;
        m_hasConnection_ = {};
    }

    constexpr uint16_t port() const noexcept
    { return m_port_; }

    void setExpectTimeout(bool const expectTimeout)
    { m_expectTimeout_ = expectTimeout; }

    bool waitForConnection() {
        std::unique_lock lock {m_readyMutex_};
        return m_serverReady_.wait_for(lock, std::chrono::seconds{5}, [this]{ return m_hasConnection_; });
    }

private:
    template<typename T>
    void run(const T &name) {
        using namespace std::chrono_literals;

        ServerType server{};
        if (!server.listen(name)) {
            qDebug() << "Error listening:" << server.serverError();
            return;
        }
        assert(server.isListening());

        {
            std::scoped_lock lock { m_readyMutex_ };
            if constexpr (std::is_same_v<ServerType, QTcpServer>) {
                m_port_ = server.serverPort();
            } else {
                m_port_ = 1;
            }
        }

        SocketType * conn {};
        m_serverReady_.notify_all();
        for (int i {}; i < 10 && !m_stop_; ++i) {
            if (server.waitForNewConnection(1000)) {
                conn = server.nextPendingConnection();
                break;
            }
        }

        if (!conn) {
            if (!m_expectTimeout_) { QFAIL("No incoming connection within timeout!"); }
            m_port_ = 0;
            return;
        }

        m_hasConnection_ = true;
        m_serverReady_.notify_all();

        if (conn->waitForReadyRead(10000)) {
            const auto request = conn->readLine();
            qDebug() << request;
            if (request == "GET /stream HTTP/1.1\r\n") {
                QStringList lines{};
                for (int i {}; i < 10; ++i) { lines.push_back(QStringLiteral("Hola %1\n").arg(i)); }

                auto const len {
                    std::accumulate(lines.cbegin(), lines.cend(), 0,
                                    [](int l, const QString &s) { return l + s.size(); }) };
                conn->write("HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/plain\r\n"
                            "Content-Length: " +
                            QByteArray::number(len) +
                            "\r\n"
                            "\r\n");
                conn->flush();
                for (auto const & line : lines) {
                    conn->write(line.toUtf8());
                    conn->flush();
                    std::this_thread::sleep_for(100ms);
                }
            } else {
                if (request == "GET /block HTTP/1.1\r\n") {
                    std::this_thread::sleep_for(500ms);
                }

                conn->write("HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/plain\r\n"
                            "\r\n"
                            "abcdef");
            }
            conn->flush();
            conn->close();
        } else if (!m_stop_) {
            if (conn->state() == std::remove_cvref_t<decltype(*conn)>::ConnectedState) {
                if (!m_expectTimeout_) {
                    QFAIL("No request within 10 seconds");
                }
            } else {
                qDebug() << "Client disconnected without sending request";
            }
        }

        delete conn;
        m_port_ = 0;
    }
};
