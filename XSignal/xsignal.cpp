#include "xsignal.hpp"
#include <unistd.h>
#include <string>
#include <sstream>
#include <unordered_map>

XTD_NAMESPACE_BEGIN

class XSignal_Impl final: public XSignal {

    X_DISABLE_COPY_MOVE(XSignal_Impl)

    struct Private{ explicit Private() = default; };

    static inline void no_sig_(const int &sig){
        std::stringstream msg{};
        msg << "This signal(id: " << sig << ")" << "is not registered\n";
        write(STDERR_FILENO,msg.str().c_str(),msg.str().length());
    }

    static void signal_handler(const int sig,siginfo_t* const info,void* const ctx) {

        const auto &it{sm_callable_map_.find(sig)};
        if (sm_callable_map_.end() == it || !it->second || !it->second->m_call_){
            no_sig_(sig);
            return;
        }

        const auto &this_{it->second};
        this_->m_context_ = ctx;
        this_->m_sig_ = sig;
        this_->m_info_ = *info;
        this_->m_call_->call();
    }

    void Unregister_helper() {
        if (m_sig_ > 0) {
            m_act_.sa_handler = SIG_DFL;
            m_act_.sa_flags = 0;
            sigaction(m_sig_, &m_act_,{});
            m_sig_ = -1;
        }
    }

    void set_call(const Callable_Ptr &p) override{
        m_call_ = std::forward<decltype(p)>(p);
    }

public:
    [[nodiscard]] int sig() const & override {
        return m_sig_;
    }

    [[nodiscard]] const siginfo_t &siginfo() const & override {
        return m_info_;
    }

    [[nodiscard]] ucontext_t* context() const & override {
        return static_cast<ucontext_t*>(m_context_);
    }

    void Unregister() override {
        Unregister_helper();
        sm_callable_map_.erase(m_sig_);
    }

    static void Unregister(const int &sig){
        if (const auto &it{sm_callable_map_.find(sig)};sm_callable_map_.end() != it){
            it->second->Unregister();
        }
    }

    static auto siginfo(const int &sig) {
        const auto &it{sm_callable_map_.find(sig)};
        return  sm_callable_map_.end() != it ? it->second->m_info_ : siginfo_t{};
    }

    using XSignal_Impl_Ptr = std::shared_ptr<XSignal_Impl>;

private:
    using call_map_t = std::unordered_map<int,XSignal_Impl_Ptr>;
    static inline call_map_t sm_callable_map_{};
    int m_sig_{-1};
    struct sigaction m_act_{};
    siginfo_t m_info_{};
    void *m_context_{};
    Callable_Ptr m_call_{};

public:
    explicit XSignal_Impl(const int &sig,const int &flags,Private):m_sig_(sig){
        m_act_.sa_sigaction = signal_handler;
        m_act_.sa_flags = SA_SIGINFO | flags;
        sigaction(sig, &m_act_,{});
    }

    static XSignal_Impl_Ptr create(const int &sig,const int &flags){
        if (sig <= 0 || flags < 0){
            return {};
        }

        try{
            const auto obj{std::make_shared<XSignal_Impl>(sig,flags,Private{})};
            sm_callable_map_[sig] = obj;
            return obj;
        }catch (const std::exception &){
            return {};
        }
    }

    ~XSignal_Impl() override {
        Unregister_helper();
    }
};

siginfo_t XSignal::siginfo(const int &sig){
    return XSignal_Impl::siginfo(sig);
}

void XSignal::Unregister(const int &sig){
    XSignal_Impl::Unregister(sig);
}

bool XSignal::Send_signal(const int &pid_,const int &sig_,const sigval &val_){
#if defined(__APPLE__) || defined(__MACH__)
    (void)val_;
    return !kill(pid_,sig_);
#else
    return !sigqueue(pid_,sig_,val_);
#endif
}

Signal_Ptr XSignal::create(const int &sig,const int &flags){
    return XSignal_Impl::create(sig,flags);
}

XTD_NAMESPACE_END
