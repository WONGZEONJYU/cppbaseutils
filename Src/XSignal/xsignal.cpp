#include <xsignal_p_p.hpp>
#include <unistd.h>
#include <sstream>
#include <functional>
#include <XHelper/xhelper.hpp>
#include <XLog/xlog.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

void XSignalPrivate::noSig(int const sig) {
    std::stringstream msg {};
    msg << "This signal(id: " << sig << ")" << "is not registered\n";
    auto const len{ write(STDERR_FILENO,msg.str().c_str(),msg.str().length()) };
    (void)len;
}

void XSignalPrivate::signalHandler(int const sig, siginfo_t * const info, void * const ctx) {
    auto const async{ m_async_.loadAcquire() };
    if (!async) { return; }
    X_RAII r{ [&async]() noexcept{ async->ref(); }
        ,[&async]() noexcept{ async->deref(); } };
    async->emitSignal(sig,info,ctx);
}

XSignalPrivate::~XSignalPrivate()
{ unregisterHelper(); }

void XSignalPrivate::registerHelper(int const sig, int const flags) noexcept {
    m_sig = sig;
    m_act_.sa_sigaction = signalHandler;
    m_act_.sa_flags = SA_SIGINFO | flags;
    sigaction(sig, &m_act_,{});
}

void XSignalPrivate::unregisterHelper() noexcept {
    if (m_sig > 0) {
        m_act_.sa_handler = SIG_DFL;
        m_act_.sa_flags = 0;
        sigaction(m_sig, std::addressof(m_act_),{});
        m_sig = -1;
    }
}

XSignal::XSignal() = default;

void XSignal::registerHelper(SignalPtr const & p)
{

}

XSignal::~XSignal() = default;

bool XSignal::construct_(int const sig, int const flags) noexcept {
    if (sig <= 0 || flags < 0)
    { return {}; }
    if (m_d_ptr_ = makeUnique<XSignalPrivate>(this);!m_d_ptr_)
    { return {}; }
    d_func()->registerHelper(sig,flags);
    return true;
}

void XSignal::setCall_(XCallableHelper::CallablePtr && f) noexcept
{ X_D(XSignal); d->m_callable_.swap(f); }

void XSignal::unregister()
{ X_D(XSignal); d->unregisterHelper();}

// siginfo_t XSignal::siginfo(int const sig) {
//     const auto it { XSignalPrivate::sm_signalMap_.find(sig)} ;
//     return XSignalPrivate::sm_signalMap_.end() != it
//         ? *it->second->d_func()->m_info
//         : siginfo_t {};
// }

void XSignal::unregister(int const sig) {
    // if (auto const it { XSignalPrivate::sm_signalMap_.find(sig) }
    //     ;XSignalPrivate::sm_signalMap_.end() != it) {
    //     it->second->unregister();
    //     XSignalPrivate::sm_signalMap_.erase(it);
    // }
}

void SignalUnregister(int const sig)
{ XSignal::unregister(sig); }

bool emitSignal(int const pid_, int const sig_, sigval const & val_) noexcept{
#if defined(__APPLE__) || defined(__MACH__)
    (void)val_;
    return !kill(pid_,sig_);
#else
    return !sigqueue(pid_,sig_,val_);
#endif
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
