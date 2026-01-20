#ifndef XUTILS2_Q_CORO_THREAD_HPP
#define XUTILS2_Q_CORO_THREAD_HPP 1

#pragma once

#include <XQtHelper/qcoro/core/qcorosignal.hpp>
#include <chrono>
#include <QPointer>
#include <QThread>
#include <QEvent>
#include <QCoreApplication>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail { class QCoroThread; }

inline detail::QCoroThread qCoro(QThread * ) noexcept;
inline detail::QCoroThread qCoro(QThread & ) noexcept;

namespace detail {

    class ContextHelper : public QObject {
        Q_OBJECT
        QThread * m_thread_ {};
        std::coroutine_handle<> m_awaiter_ {};

    public:
        inline static const auto eventType { static_cast<QEvent::Type>(QEvent::registerEventType()) };

        explicit ContextHelper(std::coroutine_handle<> const awaiter, QThread * const thread) noexcept
            : m_thread_ {thread} , m_awaiter_ {awaiter} {   }

        bool event(QEvent * const e) override {
            if (e->type() != eventType) { return QObject::event(e); }
            Q_ASSERT(QThread::currentThread() == m_thread_);
            m_awaiter_.resume();
            return true;
        }
    };

    struct ThreadContextPrivate {
        explicit(false) constexpr ThreadContextPrivate(QThread * const thread) noexcept
            : m_thread { thread } {}
        QThread * m_thread {};
        std::unique_ptr<ContextHelper> m_context;
    };

    class QCoroThread {
        QPointer<QThread> m_thread_{};
    public:
        explicit(false) QCoroThread(QThread * const thread) noexcept
            : m_thread_ { thread } {    }

        using milliseconds = std::chrono::milliseconds;

        XCoroTask<bool> waitForStarted(milliseconds const timeout = milliseconds{-1}) const {
            if (m_thread_->isRunning()) { co_return true; }
            if (m_thread_->isFinished()) { co_return false; }
            auto const result { co_await qCoro(m_thread_.data(), &QThread::started, timeout) };
            co_return result.has_value();
        }

        XCoroTask<bool> waitForFinished(milliseconds const timeout = milliseconds{-1}) const {
            if (m_thread_->isFinished()) { co_return true; }
            if (!m_thread_->isRunning()) { co_return false; }
            auto const result { co_await qCoro(m_thread_.data(), &QThread::finished, timeout) };
            co_return result.has_value();
        }
    };

}

class ThreadContext {
    std::unique_ptr<detail::ThreadContextPrivate> m_d_ {};
public:
    explicit(false) constexpr ThreadContext(QThread * const thread)
        : m_d_{ std::make_unique<detail::ThreadContextPrivate>(thread) }
    { }

    virtual ~ThreadContext() = default;

    Q_DISABLE_COPY(ThreadContext)

#ifdef Q_CC_GNU
    // Workaround for the a GCC bug(?) where GCC tries to move the ThreadContext
    // into QCoro::TaskPromise::await_transform() (most likely).
    ThreadContext(ThreadContext &&) noexcept = default;
#else
    ThreadContext(ThreadContext &&) noexcept = delete;
#endif
    ThreadContext &operator=(ThreadContext &&) = delete;

    static constexpr bool await_ready() noexcept { return {}; }

    void await_suspend(std::coroutine_handle<> const awaiter) const noexcept {
        m_d_->m_context = std::make_unique<detail::ContextHelper>(awaiter, m_d_->m_thread);
        m_d_->m_context->moveToThread(m_d_->m_thread);
        qCoro(m_d_->m_thread).waitForStarted().then([this]{
            auto event { std::make_unique<QEvent>(static_cast<QEvent::Type>(detail::ContextHelper::eventType)) };
            QCoreApplication::postEvent(m_d_->m_context.get(), event.release());
        });
    }

    static constexpr void await_resume() noexcept {}
};

inline ThreadContext moveToThread(QThread * const thread)
{ return {thread}; }

inline detail::QCoroThread qCoro(QThread * const thread) noexcept
{ return { thread }; }

inline detail::QCoroThread qCoro(QThread & thread) noexcept
{ return { std::addressof(thread) }; }

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif