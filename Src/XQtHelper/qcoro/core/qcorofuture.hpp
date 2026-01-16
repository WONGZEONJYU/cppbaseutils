#ifndef XUTILS2_Q_CORO_FUTURE_HPP
#define XUTILS2_Q_CORO_FUTURE_HPP 1

#pragma once

#include <XGlobal/xclasshelpermacros.hpp>
#include <XGlobal/xversion.hpp>
#include <XCoroutine/xcoroutinetask.hpp>
#include <memory>

#include <QFuture>
#include <QFutureWatcher>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    template<typename T>
    class QCoroFuture final {

        QFuture<T> m_future_ {};

        friend struct awaiter_type<QFuture<T>>;

        template<typename Tp>
        struct WaitForFinishedOperationAbstract {
        protected:
            QFuture<Tp> m_future_ {};

        public:
            Q_DISABLE_COPY(WaitForFinishedOperationAbstract);
            X_DEFAULT_MOVE(WaitForFinishedOperationAbstract)

            [[nodiscard]] bool await_ready() const noexcept
            { return m_future_.isFinished() || m_future_.isCanceled(); }

            void await_suspend(std::coroutine_handle<> const h) {
                auto const watcher { std::make_unique<QFutureWatcher<Tp>>().release() };
                QObject::connect(watcher, &QFutureWatcherBase::finished, [watcher, h]{
                    watcher->deleteLater();
                    h.resume();
                });
                watcher->setFuture(m_future_);
            }

        protected:
            explicit(false) constexpr WaitForFinishedOperationAbstract(QFuture<Tp> const & future)
                : m_future_ { future }  {}
        };

        struct WaitForFinishedOperationType : WaitForFinishedOperationAbstract<T> {
            explicit(false) constexpr WaitForFinishedOperationType(QFuture<T> const & future)
                : WaitForFinishedOperationAbstract<T> { future } {}
            T await_resume() const { return this->m_future_.result(); }
        };

        struct WaitForFinishedOperationVoid : WaitForFinishedOperationAbstract<void> {
            explicit(false) constexpr WaitForFinishedOperationVoid(QFuture<void> const & future)
                : WaitForFinishedOperationAbstract<void> { future } {  }
            void await_resume() noexcept(false) {
                // This won't block, since we know for sure that the QFuture is already finished.
                // The weird side-effect of this function is that it will re-throw the stored
                // exception.
                this->m_future_.waitForFinished();
            }
        };

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        template<typename Tp = T> requires (!std::is_void_v<Tp>)
        struct TakeResultOperation : WaitForFinishedOperationAbstract<Tp> {
            explicit(false) constexpr TakeResultOperation(QFuture<Tp> const & future)
                : WaitForFinishedOperationAbstract<Tp> { future } {}
            Tp await_resume() { return this->m_future_.takeResult(); }
        };
#endif

        using WaitForFinishedOperation = std::conditional_t<
            std::is_void_v<T>, WaitForFinishedOperationVoid, WaitForFinishedOperationType>;

    public:
        explicit(false) constexpr QCoroFuture(QFuture<T> const & future)
            : m_future_ {future} { }

        XCoroTask<T> waitForFinished()
        { co_return co_await WaitForFinishedOperation { m_future_ }; }

        XCoroTask<T> result()
        { co_return co_await WaitForFinishedOperation { m_future_ }; }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        XCoroTask<T> takeResult() requires (!std::is_void_v<T>)
        { co_return std::move(co_await TakeResultOperation<> { m_future_ }); }
#endif
    };

    template<typename T>
    struct awaiter_type<QFuture<T>> { using type = QCoroFuture<T>::WaitForFinishedOperation; };

}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
