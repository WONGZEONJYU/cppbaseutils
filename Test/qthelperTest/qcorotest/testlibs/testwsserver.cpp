#include <testwsserver.hpp>
#include <chrono>
#include <mutex>
#include <thread>
#include <QWebSocket>
#include <QWebSocketServer>
#include <QDebug>
#include <QTimer>
#include <QRandomGenerator>
#include <QTest>
#include <XAtomic/xatomic.hpp>

class Server : public QObject {
    Q_OBJECT
    std::unique_ptr<QWebSocket> m_socket_{};
    std::unique_ptr<QWebSocketServer> m_server_{};
    std::unique_ptr<QTimer> m_timeout_{};
    std::condition_variable m_cond_{};
    std::mutex m_mutex_{};
    XUtils::XAtomicBool m_expectTimeout_{};
    QUrl mUrl{};

public:
    constexpr Server() noexcept = default;

    void setExpectTimeout() noexcept
    { m_expectTimeout_.storeRelaxed(true); }

    QUrl waitForStart() {
        std::unique_lock lock {m_mutex_};
        m_cond_.wait(lock, [this]{ return !mUrl.isEmpty(); });
        return mUrl;
    }

    bool waitForConnection() {
        std::unique_lock lock {m_mutex_};
        if (!m_socket_) {
            using namespace std::chrono_literals;
            return m_cond_.wait_for(lock, 5s, [this]{ return static_cast<bool>(m_socket_); });
        }
        return true;
    }

    void start() {
        m_server_ = std::make_unique<QWebSocketServer>(QStringLiteral("QCoroTestWSServer"), QWebSocketServer::NonSecureMode);
        if (!m_server_->listen(QHostAddress::LocalHost)) {
            qCritical() << "WebSocket server failed to start listening";
            close();
            return;
        }

        std::scoped_lock lock {m_mutex_};
        mUrl = m_server_->serverUrl();

        m_timeout_ = std::make_unique<QTimer>();
        m_timeout_->callOnTimeout(this,&Server::newConnectionTimeout);
        m_timeout_->setSingleShot(true);

        {
            using namespace std::chrono_literals;
            m_timeout_->start(10s);
        }

        connect(m_server_.get(), &QWebSocketServer::newConnection,
                this, &Server::newConnection);

        connect(m_server_.get(), &QWebSocketServer::acceptError,
                this, [this]<typename Err>(Err && error) {
                    qCritical() << "WebSocket server failed to accept incoming connection:" << std::forward<Err>(error);
                    close();
                });

        connect(m_server_.get(), &QWebSocketServer::serverError,
                this, [this]<typename Err>(Err && error) {
                    qCritical() << "WebSocket server failed to set up WS connection" << std::forward<Err>(error);
                    close();
                });

        m_cond_.notify_all();
    }

    void close() {
        QThread::currentThread()->quit();
        m_socket_.reset();
        m_timeout_.reset();
        m_server_.reset();
    }

private Q_SLOTS:
    void newConnectionTimeout() {
        if (!m_expectTimeout_.loadRelaxed()) { QFAIL("No incoming connection within timeout"); }
        close();
    }

    void newConnection() {
        m_timeout_->stop();

        {
            std::scoped_lock lock{m_mutex_};
            m_socket_.reset(m_server_->nextPendingConnection());
        }
        m_cond_.notify_all();

        m_timeout_ = std::make_unique<QTimer>();

        m_timeout_->callOnTimeout(this, [this]{
            if (!m_expectTimeout_.loadRelaxed()) {
                QFAIL("No incoming request within timeout");
            }
            close();
        });

        m_timeout_->setSingleShot(true);

        {
            using namespace std::chrono_literals;
            m_timeout_->start(5s);
        }

        connect(m_socket_.get(), &QWebSocket::textMessageReceived,this
            , [this](QString const & msg){
                    m_timeout_->stop();
                    if (auto const request{ m_socket_->requestUrl().path()}
                        ;request == QLatin1String("/delay"))
                    {
                        using namespace std::chrono_literals;
                        std::this_thread::sleep_for(300ms);
                        m_socket_->sendTextMessage(msg);
                    } else if (request == QLatin1String("/large")) {
                        const auto response { QString::fromLatin1(generateLargeMessage().toHex()) };
                        m_socket_->sendTextMessage(response);
                    } else {
                        m_socket_->sendTextMessage(msg);
                    }
            }
        );

        connect(m_socket_.get(), &QWebSocket::binaryMessageReceived,this
            , [this](QByteArray const & msg){
                m_timeout_->stop();

                if (auto const request { m_socket_->requestUrl().path() };
                    request == QLatin1String("/delay"))
                {
                    using namespace std::chrono_literals;
                    std::this_thread::sleep_for(100ms);
                    m_socket_->sendBinaryMessage(msg);
                } else if (request == QLatin1String("/large")) {
                    m_socket_->sendBinaryMessage(generateLargeMessage());
                } else {
                    m_socket_->sendBinaryMessage(msg);
                }
            }
        );
    }

private:
    static QByteArray generateLargeMessage() {
        qsizetype constexpr size { 10 * 1024 * 1024 }; /* 10MiB */
        std::vector<uint64_t> buffer ( size,0 );
        QRandomGenerator::global()->fillRange(buffer.data(), buffer.size());
        QByteArray msg {size ,0};
        std::memcpy(msg.data(), buffer.data(), buffer.size());
        return msg;
    }
};

TestWsServer::TestWsServer() = default;

TestWsServer::~TestWsServer() = default;

void TestWsServer::start() {
    m_thread_ = std::make_unique<QThread>();
    m_thread_->start();

    m_server_ = std::make_unique<Server>();

    m_server_->moveToThread(m_thread_.get());

    QMetaObject::invokeMethod(m_server_.get(), &Server::start, Qt::QueuedConnection);

    m_url_ = m_server_->waitForStart();
}

void TestWsServer::stop() {
    if (m_thread_ && m_thread_->isRunning()) {
        QMetaObject::invokeMethod(m_server_.get(), &Server::close, Qt::BlockingQueuedConnection);
        m_thread_->wait();
    }
    m_thread_.reset();
    m_server_.reset();
    m_url_.clear();
}

QUrl TestWsServer::url() const noexcept
{ return m_url_; }

void TestWsServer::setExpectTimeout() const noexcept
{ m_server_->setExpectTimeout(); }

bool TestWsServer::waitForConnection() const
{ return m_server_->waitForConnection(); }

#include "testwsserver.moc"
