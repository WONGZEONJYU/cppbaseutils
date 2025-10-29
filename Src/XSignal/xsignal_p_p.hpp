#ifndef X_SIGNAL_P_P_HPP
#define X_SIGNAL_P_P_HPP 1

#include <XSignal/xsignal_p.hpp>
#include <map>
#include <list>
#include <thread>
#include <shared_mutex>

extern "C" {
#include <unistd.h>
#include <sys/types.h>
#ifdef X_PLATFORM_MACOS
#include <sys/time.h>
#include <sys/event.h>
#endif
#ifdef X_PLATFORM_LINUX
#include <sys/epoll.h>
#endif
}

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
    int m_pipeFd_[2]{-1,-1};
#ifdef X_PLATFORM_MACOS
    struct macosPlatform {
        int m_kqueue_{-1};
        kevent64_s m_pipeEvent_{};
    }m_mac_{};
#endif
#ifdef X_PLATFORM_LINUX
    struct linuxPlatform {
        int m_epollFd{};
    }m_linux_{};
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
    void deConstruct() noexcept;
    bool construct_() noexcept;
    void signalHandler();
    bool wait() noexcept;
    void readAndCall();
};

inline bool XSignalPrivate::SignalAsynchronously::construct_() noexcept {
    if ( pipe(m_pipeFd_) < 0 ) { return {}; }
#ifdef X_PLATFORM_MACOS
    {
        auto const fd{ kqueue() };
        if (fd < 0) { return {}; }
        m_mac_.m_kqueue_ = fd;
        EV_SET64(std::addressof(m_mac_.m_pipeEvent_)
            ,m_pipeFd_[0]
            ,EVFILT_READ
            ,EV_ADD | EV_ENABLE
            ,0,0,0,0,0);
    }
#endif

#ifdef X_PLATFORM_LINUX
    {
        auto const epollFd{ epoll_create1(0) };
        if (epollFd < 0) { return {}; }
        m_linux_.m_epollFd = epollFd;

        epoll_event ev{};
        ev.data.fd = m_pipeFd_[0];
        ev.events = EPOLLIN;

        if (epoll_ctl(epollFd,EPOLL_CTL_ADD
            ,m_pipeFd_[0],std::addressof(ev)) < 0)
        { return {}; }
    }

#endif
    return true;
}

inline XSignalPrivate::SignalAsynchronously::~SignalAsynchronously()
{ deConstruct(); }

inline void XSignalPrivate::SignalAsynchronously::deConstruct() noexcept {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    stopHandler();
    for (auto & item: m_pipeFd_)
    { if (item > 0) { close(item); item = -1; } }
#ifdef X_PLATFORM_MACOS
    if (m_mac_.m_kqueue_ > 0)
    { close(m_mac_.m_kqueue_); m_mac_.m_kqueue_ = -1; }
#endif
#ifdef X_PLATFORM_LINUX
    if (m_linux_.m_epollFd > 0)
    { close(m_linux_.m_epollFd); m_linux_.m_epollFd = -1; }
#endif
}

inline void XSignalPrivate::SignalAsynchronously::emitSignal(SignalArgs const & args) const noexcept
{ write(m_pipeFd_[1],std::addressof(args),sizeof(args)); }

inline void XSignalPrivate::SignalAsynchronously::startHandler() {
    if (!m_isExit_.loadAcquire()) {
        m_isExit_.storeRelaxed(false);
        m_threadHandle_ = std::thread {[this]{signalHandler();}};
    }
}

inline void XSignalPrivate::SignalAsynchronously::stopHandler() {
    m_isExit_.storeRelaxed(true);
    if (m_threadHandle_.joinable()) { m_threadHandle_.join(); }
}

inline void XSignalPrivate::SignalAsynchronously::signalHandler() {
    while (!m_isExit_.loadAcquire())
    { if (wait()) { readAndCall(); } }
}

inline bool XSignalPrivate::SignalAsynchronously::wait() noexcept {

#ifdef X_PLATFORM_MACOS
    timespec constexpr ts {1,0};
    kevent64_s event{};
    auto const ret{ kevent64(m_mac_.m_kqueue_
        ,std::addressof(m_mac_.m_pipeEvent_),1
        ,std::addressof(event),1
        ,0,std::addressof(ts)) };

    if (ret < 0) {
        std::perror("signalKevent err");
        //std::strerror()
        if (EINTR != errno)
        { m_isExit_.storeRelease(true); }
        return {};
    }

    if (!ret ) { return {}; }

    if (EVFILT_READ != event.filter || static_cast<int>(event.ident) != m_pipeFd_[0])
    { return {}; }

    return true;
#endif

#ifdef X_PLATFORM_LINUX
    epoll_event ev{};
    auto const ret{ epoll_wait(m_linux_.m_epollFd,std::addressof(ev),1,1000) };
    if (ret < 0) {
        std::perror("epoll_wait");
        if (EINTR != errno)
        { m_isExit_.storeRelease(true); }
        return {};
    }

    if (!ret){ return {}; }

    if (m_pipeFd_[0] != ev.data.fd)
    { return {}; }

    return true;
#endif
}

inline void XSignalPrivate::SignalAsynchronously::readAndCall() {
    SignalArgs args{};

    {
        decltype(read({},{},{})) ret {};
        do {
            errno = 0;
            ret = read(m_pipeFd_[0],std::addressof(args),sizeof(args)) ;
        } while (!m_isExit_.loadAcquire() && ret < 0 && EINTR == errno);

        if ( ret < static_cast<decltype(ret)>(sizeof(args)) ) { return; }
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
