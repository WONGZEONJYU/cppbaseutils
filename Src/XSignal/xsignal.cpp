#include <future>
#include <xsignal_p_p.hpp>
#include <unistd.h>
#include <sstream>
#include <XHelper/xhelper.hpp>
#include <XLog/xlog.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

static std::mutex sm_mtx_{};

void XSignalPrivate::noSig(int const sig) {
    std::stringstream msg {};
    msg << "This signal(id: " << sig << ")" << "is not registered\n";
    auto const len{ write(STDERR_FILENO,msg.str().c_str(),msg.str().length()) };
    (void)len;
}

void XSignalPrivate::signalHandler(int const sig, siginfo_t * const info, void * const ctx) {
    auto const async{ m_async_.loadAcquire() };
    if (!async) { return; }
    SignalArgs const args { sig,info,ctx };
    async->emitSignal(args);
}

XSignalPrivate::~XSignalPrivate()
{ unregisterHelper(); }

void XSignalPrivate::registerHelper(int const sig, int const flags) noexcept {
    m_sig = sig;
    m_act_.sa_sigaction = signalHandler;
    m_act_.sa_flags = SA_SIGINFO | SA_RESTART | flags;
    sigaction(sig, &m_act_,{});
}

int XSignalPrivate::unregisterHelper() noexcept {
    auto const sig{ m_sig };
    if (m_sig > 0) {
        m_act_.sa_handler = SIG_DFL;
        m_act_.sa_flags = 0;
        sigaction(m_sig, std::addressof(m_act_),{});
        m_sig = -1;
    }
    return sig;
}

void XSignalPrivate::callHandler(SignalArgs const & args) noexcept {
    auto const & [sig,info,ctx]{ args };
    m_sig = sig;
    m_info = info;
    m_context = ctx;
    std::invoke(*m_callable_);
}

XSignal::XSignal() = default;

bool XSignal::registerHelper(SignalPtr const & p) {

    static X_RAII const r {[]() noexcept{
        auto async{ XSignalPrivate::SignalAsynchronously::Create() };
        while (!async) {
            async = XSignalPrivate::SignalAsynchronously::Create();
            std::cerr << "SignalAsynchronously created retry!" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        async->ref();
        async->startHandler();
        {
            std::unique_lock lock(sm_mtx_);
            XSignalPrivate::m_async_.storeRelease(async);
        }
    },[]() noexcept{
        if (auto const async{XSignalPrivate::m_async_.loadAcquire()}
            ; async && !async->deref())
        {
            XSignalPrivate::m_async_.storeRelease({});
            delete async;
        }
    }};

    std::unique_lock lock(sm_mtx_);
    auto const async{ XSignalPrivate::m_async_.loadAcquire() };
    if (!async) {return {};}
    async->addHandler(p->sig(),p);
    return true;
}

XSignal::~XSignal()
{ std::cerr << "sig : " << m_d_ptr_->m_sig << " destroy " << std::endl; }

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

void XSignal::unregister() {
    X_D(XSignal);
    auto const sig{ d->unregisterHelper() };
    std::unique_lock lock(sm_mtx_);
    auto const async{ XSignalPrivate::m_async_.loadAcquire() };
    if (!async) { return; }
    async->ref();
    async->removeHandler(sig);
    async->deref();
}

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
