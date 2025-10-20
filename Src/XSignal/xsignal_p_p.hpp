#ifndef X_SIGNAL_P_P_HPP
#define X_SIGNAL_P_P_HPP 1

#include <XSignal/xsignal_p.hpp>
#include <map>
#include <list>
#include <thread>
#include <shared_mutex>
#include <unistd.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

struct XSignalPrivate::SignalArgs final {
    int64_t m_sig{};
    siginfo_t * m_info{};
    void * m_ctx {};
    constexpr SignalArgs() = default;
    constexpr SignalArgs(int const sig, siginfo_t * const info, void * const ctx) :
    m_sig(sig), m_info(info), m_ctx(ctx) {}
};

static_assert(std::is_trivially_copyable_v<XSignalPrivate::SignalArgs>,"SignalArgs must be trivially copyable");

class XSignalPrivate::SignalAsynchronously final
    : XSingleton<SignalAsynchronously>
{
    X_TWO_PHASE_CONSTRUCTION_CLASS
    friend class XSignal;

    std::map<int,SignalPtr> m_signalHandlerMap_{};
    std::list<SignalPtr> m_orphanSignals_{};
    std::thread m_threadHandle_{};
    std::shared_mutex m_mtx_{};
    XAtomicBool m_isExit_{};
    XAtomicInt m_ref_{};
#ifdef X_PLATFORM_MACOS
    struct macosPlatform {
        int m_kqueue_{-1},m_pipeFd_[2]{-1,-1};
        kevent64_s m_pipeEvent_{};
    }m_mac_{};
#endif
#ifdef X_PLATFORM_LINUX
    struct linuxPlatform {
        int m_epollFd{};
    }
#endif
public:
    void addHandler(int, SignalPtr);
    void removeHandler(int);
    void ref() noexcept { m_ref_.ref(); }
    bool deref() noexcept { return m_ref_.deref(); }
    void emitSignal(SignalArgs const &) const noexcept;
    void startHandler();
    void stopHandler();

private:
    explicit constexpr SignalAsynchronously() = default;
    ~SignalAsynchronously();
    bool construct_() noexcept;
    void signalHandler();
    void readAndCall();
};

inline bool XSignalPrivate::SignalAsynchronously::construct_() noexcept {

    if (m_mac_.m_kqueue_ = kqueue();m_mac_.m_kqueue_ < 0) { return {}; }

    if ( pipe(m_mac_.m_pipeFd_) < 0 ) { return {}; }

    EV_SET64(std::addressof(m_mac_.m_pipeEvent_)
        ,m_mac_.m_pipeFd_[0]
        ,EVFILT_READ
        ,EV_ADD | EV_ENABLE
        ,0,0,0,0,0);

    return true;
}

inline XSignalPrivate::SignalAsynchronously::~SignalAsynchronously() {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    stopHandler();
    if (m_threadHandle_.joinable()) { m_threadHandle_.join(); }
    for (auto const & item: m_mac_.m_pipeFd_) { if (item > 0) { close(item); } }
    if (m_mac_.m_kqueue_ > 0) { close(m_mac_.m_kqueue_); }
}

inline void XSignalPrivate::SignalAsynchronously::emitSignal(SignalArgs const & args) const noexcept
{ write(m_mac_.m_pipeFd_[1],std::addressof(args),sizeof(args)); }

inline void XSignalPrivate::SignalAsynchronously::startHandler() {
    if (!m_isExit_.loadAcquire()) {
        m_isExit_.storeRelaxed(false);
        m_threadHandle_ = std::thread {[this]{signalHandler();}};
    }
}

inline void XSignalPrivate::SignalAsynchronously::stopHandler()
{ m_isExit_.storeRelaxed(true); }

inline void XSignalPrivate::SignalAsynchronously::signalHandler() {
    while (!m_isExit_.loadAcquire()) {

        timespec constexpr ts {1,0};

#ifdef X_PLATFORM_MACOS
        kevent64_s event{};
        if (auto const ret{ kevent64(m_mac_.m_kqueue_
            ,std::addressof(m_mac_.m_pipeEvent_),1
            ,std::addressof(event),1
            ,0,std::addressof(ts)) };ret < 0)
        {
            std::perror("signalKevent err");
            //std::strerror()
            if (EINTR != errno) { m_isExit_.storeRelease(true); }
            continue;
        }else if (!ret){
            continue;
        }
        if (EVFILT_READ != event.filter
            || static_cast<int>(event.ident) != m_mac_.m_pipeFd_[0])
        { continue; }
#endif

#ifdef X_PLATFORM_LINUX



#endif
        readAndCall();
    }
}

inline void XSignalPrivate::SignalAsynchronously::readAndCall() {
    SignalArgs args{};

    if (read(m_mac_.m_pipeFd_[0],std::addressof(args),sizeof(SignalArgs))
        < static_cast<ssize_t>(sizeof(SignalArgs)) )
    {
        std::perror("read SignalArgs error");
        return;
    }

    SignalPtr ptr{};
    {
        std::shared_lock lock(m_mtx_);
        if (auto const it { m_signalHandlerMap_.find(static_cast<int>(args.m_sig))}
            ;m_signalHandlerMap_.end() != it )
        { ptr = it->second; }
    }

    if (ptr) { ptr->d_func()->callHandler(args); }
}

inline void XSignalPrivate::SignalAsynchronously::addHandler(int const sig,SignalPtr p)
{ std::unique_lock lock(m_mtx_); m_signalHandlerMap_.insert({sig,std::move(p)}); }

inline void XSignalPrivate::SignalAsynchronously::removeHandler(int const sig) {
    SignalPtr ptr { };
    std::unique_lock lock(m_mtx_);
    auto const it = m_signalHandlerMap_.find(sig);
    if (it != m_signalHandlerMap_.end()) { return; }
    ptr.swap(it->second);
    m_signalHandlerMap_.erase(sig);
    m_orphanSignals_.push_back(std::move(ptr));
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
