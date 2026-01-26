#ifndef XUTILS2_TEST_WS_SERVER_HPP
#define XUTILS2_TEST_WS_SERVER_HPP 1

#pragma once

#include <QThread>
#include <QUrl>
#include <memory>

class Server;

class TestWsServer {

    std::unique_ptr<QThread> m_thread_{};
    std::unique_ptr<Server> m_server_;
    QUrl m_url_{};

public:
    explicit(false) TestWsServer();
    ~TestWsServer();

    void start();
    void stop();
    [[nodiscard]] QUrl url() const noexcept;
    [[nodiscard]] bool waitForConnection() const;
    void setExpectTimeout() const noexcept;
};

#endif
