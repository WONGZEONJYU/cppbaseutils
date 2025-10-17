#ifndef X_SIGNAL_P_P_HPP
#define X_SIGNAL_P_P_HPP 1

#include <XSignal/xsignal_p.hpp>
#include <map>
#include <thread>
#include <shared_mutex>
#include <unistd.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

using SignalArgs = std::tuple<int,siginfo_t * ,void*>;

class XSignalPrivate::SignalAsynchronously final {
    std::map<int,SignalPtr> m_signalHandlerMap_{};
    std::thread m_threadHandle_{};
    std::shared_mutex m_mtx_{};
    XAtomicBool m_isExit_{};
#ifdef X_PLATFORM_MACOS
    struct macosPlatform {
        int m_kqueue_{-1},m_pipeFd_[2]{-1,-1};
        kevent64_s m_pipeEvent_{};
    }m_mac_{};
#endif
#ifdef X_PLATFORM_LINUX
    int m_epollFd{};
#endif
public:
    void addHandler(int, SignalPtr);
    void removeHandler(int);

private:
    explicit constexpr SignalAsynchronously() = default;
    bool construct_() noexcept;
    ~SignalAsynchronously();
    void signalHandler();
    static void signalHandlerHandler();
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
    for (auto const & item: m_mac_.m_pipeFd_) { if (item > 0) { close(item); } }
    if (m_threadHandle_.joinable()) { m_threadHandle_.join(); }
}

inline void XSignalPrivate::SignalAsynchronously::signalHandler() {
    while (!m_isExit_.loadAcquire()) {
#ifdef X_PLATFORM_MACOS
        kevent64_s event{};
        timespec constexpr ts {1,0};
        if (auto const ret { kevent64(m_mac_.m_pipeFd_[0]
            ,std::addressof(m_mac_.m_pipeEvent_),1
            ,std::addressof(event),1
            ,0,std::addressof(ts)) };ret < 0)
        {
            perror("signalKevent err");
            if (EINTR != errno) { m_isExit_.storeRelease(true); }
            break;
        }else if (!ret){
            continue;
        }

        if (EVFILT_READ != event.filter || static_cast<int>(event.ident) != m_mac_.m_pipeFd_[0])
            {continue;}

        SignalArgs args{};

        if (read(m_mac_.m_pipeFd_[0],std::addressof(args),sizeof(SignalArgs)) < static_cast<ssize_t>(sizeof(SignalArgs)) )
            { continue;}

        auto const & [ sig,info,ctx ] { args };
        SignalPtr ptr{};
        {
            std::shared_lock lock(m_mtx_);
            if (auto const it { m_signalHandlerMap_.find(sig)} ;m_signalHandlerMap_.end() != it ) {
                ptr = it->second;
            }
        }

        if (ptr) {
            auto const pd{ ptr->d_func() };
            pd->m_sig = sig;
            pd->m_info = info;
            pd->m_context = ctx;
            std::invoke(*pd->d.m_callable_);
        }

#endif

#ifdef X_PLATFORM_LINUX
#endif
    }
}

inline void XSignalPrivate::SignalAsynchronously::addHandler(int const sig,SignalPtr p)
{ std::unique_lock lock(m_mtx_); m_signalHandlerMap_.insert({sig,std::move(p)}); }

inline void XSignalPrivate::SignalAsynchronously::removeHandler(int const sig)
{ std::unique_lock lock(m_mtx_); m_signalHandlerMap_.erase(sig); }

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
