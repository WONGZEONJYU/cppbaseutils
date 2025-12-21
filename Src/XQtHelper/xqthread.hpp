#ifndef XUTILS2_X_QTHREAD_HPP
#define XUTILS2_X_QTHREAD_HPP 1

#include <XHelper/xqt_detection.hpp>
#include <XHelper/xcallablehelper.hpp>

#ifdef HAS_QT

#include <QThread>
#include <QMutex>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XQThreadHelper : public QThread {

    Q_DISABLE_COPY(XQThreadHelper)

    CallablePtr m_callable_{};
    QMutex m_mtx_{};

public:
    explicit XQThreadHelper(QObject * const parent = {})
    : QThread { parent } {  }

    template<typename ...Args>
    explicit XQThreadHelper(QObject * const parent , Args && ...args) : QThread { parent }
    ,m_callable_ { XCallableHelper::createCallable(std::forward<Args>(args)...) }
    { }

    ~XQThreadHelper() override = default;

    template <typename ...Args>
    void setRoutine(Args && ...args) noexcept {
        QMutexLocker locker {std::addressof(m_mtx_)};
        m_callable_ = XCallableHelper::createCallable(std::forward<Args>(args)...);
    }

protected:
    template<typename T>
    static void terminate_on_exception(T && t) {
        try {
            std::forward<T>(t)();
#ifdef __GLIBCXX__
            // POSIX thread cancellation under glibc is implemented by throwing an exception
            // of this type. Do what libstdc++ is doing and handle it specially in order not to
            // abort the application if user's code calls a cancellation function.
        } catch (abi::__forced_unwind &) {
            throw;
#endif // __GLIBCXX__
        } catch (...) {
            std::terminate();
        }
    }

    void run() override {
        terminate_on_exception([this]{
            if (auto const callable{ [this]{
                QMutexLocker locker {std::addressof(m_mtx_)};
                return m_callable_;
            }()}) { (*callable)(); }
            QThread::run();
        });
    }
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
#endif
