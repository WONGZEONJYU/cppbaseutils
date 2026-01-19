#ifndef XUTILS2_TEST_WS_SERVER_HPP
#define XUTILS2_TEST_WS_SERVER_HPP 1

#pragma once

#include <QThread>
#include <QUrl>
#include <memory>

class Server;

class TestWsServer {

    std::unique_ptr<QThread> m_thread_{};
    std::unique_ptr<Server> m_server_{};
    QUrl m_url_{};

public:
    constexpr TestWsServer() = default;
    ~TestWsServer() = default;

    void start();
    void stop();
    QUrl url() const noexcept;
    bool waitForConnection() const;
    void setExpectTimeout() const noexcept;
};

#endif
