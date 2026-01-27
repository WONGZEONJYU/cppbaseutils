#include <testobject.hpp>
#include <XQtHelper/qcoro/core/qcorotimer.hpp>
#include <XQtHelper/qcoro/quick/qcoroimageprovider.hpp>
#include <QQmlEngine>

#define emit Q_EMIT // HACK: the private header uses `emit`, but QCoro is built with QT_NO_KEYWORDS
#include <private/qquickimage_p.h>
#undef emit

class TestImageProvider final: public XUtils::ImageProvider {
    bool m_async_ {};
    QString m_error_{};

public:
    Q_IMPLICIT TestImageProvider(bool const async, QString  error)
        : m_async_{async}, m_error_{std::move(error) }
    {   }

protected:
    XUtils::XCoroTask<QImage> asyncRequestImage(QString const & , QSize const & ) override {

        if (m_async_) {
            QTimer timer{};
            using namespace std::chrono_literals;
            timer.start(250ms);
            co_await timer;
        }

        co_return QImage{};
    }

};

struct QCoroImageProviderTest: QCoro::TestObject<QCoroImageProviderTest> {
    Q_OBJECT

private Q_SLOTS:
    static void testImageProvider_data() {
        QTest::addColumn<QString>("id");
        QTest::addColumn<bool>("async");
        QTest::addColumn<QString>("error");

        QTest::newRow("sync") << QStringLiteral("image-sync.jpg") << false << QString();
        QTest::newRow("async") << QStringLiteral("image-async.jpg") << true << QString();
    }

    static void testImageProvider() {
        QFETCH(QString,id);
        QFETCH(bool, async);
        QFETCH(QString, error);
        auto const source { QStringLiteral("image://qcorotest/%1.jpg").arg(id)};
        auto const provider { std::make_unique<TestImageProvider>(async, error).release() };

        QQmlEngine engine{};
        engine.addImageProvider(QStringLiteral("qcorotest"), provider);
        QVERIFY(engine.imageProvider(QStringLiteral("qcorotest")) != nullptr);

        auto const componentStr{
            QStringLiteral(R"(
                import QtQuick 2.0

                Image {
                    source: "%1";
                    asynchronous: %2;
                })").arg(source, async ? QStringLiteral("true") : QStringLiteral("false"))
        };

        QQmlComponent component { std::addressof(engine) };
        component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(QStringLiteral("")));

        std::unique_ptr<QQuickImage> const image {qobject_cast<QQuickImage *>(component.create())};
        QVERIFY(image != nullptr);

        if (async) { QTRY_COMPARE(image->status(), QQuickImage::Loading); }

        QCOMPARE(image->source(), QUrl(source));

        if (error.isEmpty()) {
            QTRY_COMPARE(image->status(), QQuickImage::Ready);
            QCOMPARE(image->progress(), 1.0);
        } else {
            QTRY_COMPARE(image->status(), QQuickImage::Error);
        }
    }
};

QTEST_MAIN(QCoroImageProviderTest)

#include "qcoroimageprovider.moc"
