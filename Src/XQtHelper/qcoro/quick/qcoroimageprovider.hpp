#ifndef XUTILS2_Q_CORO_IMAGE_PROVIDER_HPP
#define XUTILS2_Q_CORO_IMAGE_PROVIDER_HPP 1

#pragma once
#include <XCoroutine/xcoroutinetask.hpp>
#include <QQuickAsyncImageProvider>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    class QCoroImageResponse : public QQuickImageResponse {
        QImage m_image_{};
    public:
        [[nodiscard]] QQuickTextureFactory * textureFactory() const override
        { return QQuickTextureFactory::textureFactoryForImage(m_image_); }

        void reportFinished(QImage && image)
        { m_image_ = std::move(image); Q_EMIT finished(); }
    };

}

struct ImageProvider : QQuickAsyncImageProvider {

    constexpr ImageProvider() = default;
    virtual XCoroTask<QImage> asyncRequestImage(QString const & id,  QSize const & requestedSize) = 0;

private:
    QQuickImageResponse * requestImageResponse(QString const & id,  QSize const & requestedSize) override {
        auto task {asyncRequestImage(id, requestedSize)};
        auto const response { std::make_unique<detail::QCoroImageResponse>().release() };
        task.then([response](QImage && image){
            response->reportFinished(std::move(image));
        });

        return response;
    }
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
