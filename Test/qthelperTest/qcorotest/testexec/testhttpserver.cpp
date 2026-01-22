// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include <testhttpserver.hpp>
#include <QDebug>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTcpServer>
#include <QTest>
#include <QTimer>

Q_DECLARE_METATYPE(std::chrono::seconds)

using namespace std::chrono_literals;

struct TestHttpServerTest : QObject {
    Q_OBJECT
    TestHttpServer<QTcpServer> m_server_{};

private Q_SLOTS:
    void init() { m_server_.start(QHostAddress::LocalHost); }

    void cleanup() { m_server_.stop(); }

    static void testGet_data() {
        QTest::addColumn<QString>("url");
        QTest::addColumn<QByteArray>("expectedData");
        QTest::addColumn<std::chrono::seconds>("timeout");

        QTest::newRow("/") << QStringLiteral("http://localhost:%1/") << QByteArray{"abcdef"} << 5s;

        QTest::newRow("/block") << QStringLiteral("http://localhost:%1/block")
                                << QByteArray{"abcdef"} << 5s;

        QTest::newRow("/stream") << QStringLiteral("http://localhost:%1/stream")
                                 << QByteArray{"Hola 0\nHola 1\nHola 2\nHola 3\nHola 4\nHola "
                                               "5\nHola 6\nHola 7\nHola 8\nHola 9\n"}
                                 << 15s;
    }

    void testGet() {
        QFETCH(QString, url);
        QFETCH(QByteArray, expectedData);
        QFETCH(std::chrono::seconds, timeout);

        QNetworkAccessManager nam;
        QEventLoop el;

        auto const reply { std::unique_ptr<QNetworkReply>(
            nam.get(QNetworkRequest{QUrl{url.arg(m_server_.port())}})) };

        connect(reply.get(), &QNetworkReply::finished, &el, &QEventLoop::quit);

        QTimer::singleShot(timeout, &el, [&el]() mutable { el.exit(1); });

        QCOMPARE(el.exec(), 0);

        QCOMPARE(reply->readAll(), expectedData);
    }
};

QTEST_GUILESS_MAIN(TestHttpServerTest)

#include "testhttpserver.moc"
