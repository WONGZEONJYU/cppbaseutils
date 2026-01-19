#ifndef XUTILS2_Q_CORO_PROCESS_HPP
#define XUTILS2_Q_CORO_PROCESS_HPP 1

#pragma once

#include <XQtHelper/qcoro/core/qcoroiodevice.hpp>
#include <QProcess>

#ifndef QT_CONFIG
    #define QT_CONFIG(x) x
#endif

#if QT_CONFIG(process)

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    struct QCoroProcess : QCoroIODevice {

        using milliseconds = std::chrono::milliseconds;

        explicit(false) QCoroProcess(QProcess * const process)
            : QCoroIODevice { process } { }

        ~QCoroProcess() override = default;

        [[nodiscard]] XCoroTask<bool> waitForStarted(int const timeout_msecs = 30'000) const
        { return waitForStarted(milliseconds {timeout_msecs}); }

        [[nodiscard]] XCoroTask<bool> waitForStarted(milliseconds const timeout) const {
            auto const process { qobject_cast<QProcess *>(m_device_.data()) };
            if (process->state() == QProcess::Starting) {
                auto const started { co_await qCoro(process, &QProcess::started, timeout) };
                co_return started.has_value();
            }
            co_return process->state() == QProcess::Running;
        }

        [[nodiscard]] XCoroTask<bool> waitForFinished(int const timeout_msecs = 30'000) const
        { return waitForFinished(milliseconds { timeout_msecs }); }

        [[nodiscard]] XCoroTask<bool> waitForFinished(milliseconds const timeout) const {
            auto const process { qobject_cast<QProcess *>(m_device_.data()) };
            if (process->state() == QProcess::NotRunning) { co_return false; }
            auto const finished { co_await qCoro(process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), timeout) };
            co_return finished.has_value();
        }

        [[nodiscard]] XCoroTask<bool> start(QIODevice::OpenMode const mode = QIODevice::ReadWrite
            ,milliseconds const timeout = std::chrono::seconds{30}) const
        {
            qobject_cast<QProcess *>(m_device_.data())->start(mode);
            return waitForStarted(timeout);
        }

        [[nodiscard]] XCoroTask<bool> start(QString const & program, QStringList const & arguments,
                         QIODevice::OpenMode const mode = QIODevice::ReadWrite,
                         milliseconds const timeout = std::chrono::seconds{30}) const
        {
            qobject_cast<QProcess *>(m_device_.data())->start(program, arguments, mode);
            return waitForStarted(timeout);
        }
    };

}

inline auto qCoro(QProcess & p) noexcept
{ return detail::QCoroProcess {std::addressof(p)}; }

inline auto qCoro(QProcess * const p) noexcept
{ return detail::QCoroProcess{p}; }

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
#endif
