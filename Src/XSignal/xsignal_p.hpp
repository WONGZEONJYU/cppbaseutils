#ifndef XUTILS_X_SIGNAL_P_HPP
#define XUTILS_X_SIGNAL_P_HPP 1

#include <XSignal/xsignal.hpp>
#include <unordered_map>
#include <XAtomic/xatomic.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class X_CLASS_EXPORT XSignalPrivate final : public XSignalData {
    X_DECLARE_PUBLIC(XSignal)

    class SignalAsynchronously;
    static inline XAtomicPointer<SignalAsynchronously> m_async_{};

    struct sigaction m_act_{};
    XCallableHelper::CallablePtr m_callable_{};

public:
    static void noSig(int sig);
    static void signalHandler(int sig,siginfo_t * info,void * ctx);
    explicit XSignalPrivate(XSignal * const o) { m_x_ptr_ = o; }
    ~XSignalPrivate() override;
    void registerHelper(int,int) noexcept;
    void unregisterHelper() noexcept;
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
