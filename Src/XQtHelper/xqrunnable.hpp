#ifndef XUTILS2_X_Q_RUNNABLE_HPP
#define XUTILS2_X_Q_RUNNABLE_HPP 1

#include <XHelper/xqt_detection.hpp>
#include <XHelper/xcallablehelper.hpp>

#ifdef HAS_QT

#include <QRunnable>
#include <QMutex>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XQRunnable : public QRunnable {
    Q_DISABLE_COPY(XQRunnable)

    CallablePtr m_runHelper_{};
    QMutex m_mtx_{};

public:
    template<typename ... Args>
    static auto create(Args && ...args) noexcept
    { return makeUnique<XQRunnable>(std::forward<Args>(args)...).release(); }

    constexpr XQRunnable() noexcept = default;
    ~XQRunnable() override = default;

    template<typename ... Args>
    constexpr explicit XQRunnable(Args && ...args) noexcept
        : m_runHelper_ { XCallableHelper::createCallable(std::forward<Args>(args)...) }
    {}

    template<typename ...Args>
    constexpr void setRunHelper(Args && ...args) noexcept {
        QMutexLocker locker {std::addressof(m_mtx_)};
        m_runHelper_ = XCallableHelper::createCallable(std::forward<Args>(args)...);
    }

protected:
    void run() override {
        if (auto const runHelper{ [this] {
            QMutexLocker locker {std::addressof(m_mtx_)};
            return m_runHelper_;
        }()}) { (*runHelper)(); }
    }
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
#endif
