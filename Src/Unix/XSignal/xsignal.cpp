#include <xsignal_p_p.hpp>
#include <unistd.h>
#include <sstream>
#include <XHelper/xhelper.hpp>
#include <XHelper/xspace.hpp>
#include <XLog/xlog.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

static std::recursive_mutex sm_mtx_{};

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

bool XSignalPrivate::registerHelper(int const sig, int const flags) noexcept {
    auto const sigaction_{ [sig,flags,this]()noexcept{
        m_sig = sig;
        m_act_.sa_sigaction = signalHandler;
        m_act_.sa_flags = SA_SIGINFO | SA_RESTART | flags;
        errno = 0;
        auto const ret{ sigaction(sig, &m_act_,{}) };
        if (ret < 0)
        { std::perror((std::ostringstream{} << "Sig : " <<  sig << '\t' << "registerHelper sigaction").str().c_str()); }
        else
        { std::cout << "sig : " << sig << " register success" << std::endl << std::flush; }
        return !ret;
    } };

    std::unique_lock lock(sm_mtx_);
    if (auto const async{ m_async_.loadAcquire() }) {
        async->ref();
        bool ret {};
        if (!async->findHandler(sig)) { ret = sigaction_(); }
        async->deref();
        return ret;
    }
    lock.unlock();
    return sigaction_();
}

int64_t XSignalPrivate::unregisterHelper() noexcept {
    auto const sig{ m_sig };
    if (sig > 0) {
        m_act_.sa_handler = SIG_DFL;
        m_act_.sa_flags = 0;
        m_act_.sa_mask = {};
        errno = 0;
        if (auto const ret { sigaction(static_cast<int>(sig), std::addressof(m_act_),{}) }; ret < 0)
        { std::perror((std::ostringstream{} << "Sig : " << sig << '\t' << "unregisterHelper sigaction").str().c_str()); }
        else
        { std::cerr << "sig : " << sig << " unregister success" << std::endl << std::flush; }
        m_sig = -1;
    }
    return sig;
}

void XSignalPrivate::callHandler(SignalArgs const & args) noexcept {
    auto const & [sig,info,ctx]{ args };
    m_sig = static_cast<int>(sig);
    m_info = info;
    m_context = ctx;
    try {
        std::invoke(*m_callable_);
    } catch (std::exception const & e) {
        std::cerr << "sig : " << sig << " callback err : "
            << e.what() << std::endl << std::flush;
    } catch (...) {
        std::cerr << "sig : " << sig
            << " callback err : unknown error!" << std::endl << std::flush;
    }
}

XSignal::XSignal() = default;

bool XSignal::registerHelper(SignalPtr const & p) {

    static XSpace const r { []() noexcept{
        auto const async{ XSignalPrivate::SignalAsynchronously::UniqueConstruction() };
        async->ref();
        async->startHandler();
        {
            std::unique_lock lock(sm_mtx_);
            XSignalPrivate::m_async_.storeRelease(async.get());
        }
    },[]() noexcept{
        if (auto const async{XSignalPrivate::m_async_.loadAcquire()}
            ; async && !async->deref())
        {
            std::unique_lock lock(sm_mtx_);
            XSignalPrivate::m_async_.storeRelease({});
        }
    }};

    std::unique_lock lock(sm_mtx_);
    auto const async{ XSignalPrivate::m_async_.loadAcquire() };
    if (!async) { return {}; }
    async->addHandler(p->sig(),p);
    return true;
}

XSignal::~XSignal() = default;

bool XSignal::construct_(int const sig, int const flags) noexcept {
    if (sig <= 0 || flags < 0) { return {}; }
    if (m_d_ptr_ = makeUnique<XSignalPrivate>(this);!m_d_ptr_) { return {}; }
    return d_func()->registerHelper(sig,flags);
}

void XSignal::setCall_(XCallableHelper::CallablePtr && f) noexcept
{ d_func()->m_callable_.swap(f); }

void XSignal::unregister() {
    auto const sig{ d_func()->unregisterHelper() };
    std::unique_lock lock(sm_mtx_);
    auto const async{ XSignalPrivate::m_async_.loadAcquire() };
    if (!async) { return; }
    async->ref();
    async->removeHandler(sig);
    async->deref();
}

void XSignal::SignalUnRegister(int const sig) noexcept {
    std::unique_lock lock(sm_mtx_);
    auto const async{ XSignalPrivate::m_async_.loadAcquire() };
    if (!async) { return; }
    async->ref();
    if (auto const item{ async->findHandler(sig) }) { item->unregister(); }
    async->deref();
}

void SignalUnRegister(int const sig) noexcept
{ XSignal::SignalUnRegister(sig); }

bool emitSignal(int const pid_, int const sig_, sigval const & val_) noexcept {
#ifdef X_PLATFORM_MACOS
    (void)val_;
    return !kill(pid_,sig_);
#endif

#ifdef X_PLATFORM_LINUX
    return !sigqueue(pid_,sig_,val_);
#endif
}

bool emitSignal(int const sig) noexcept
{ return !std::raise(sig); }

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
