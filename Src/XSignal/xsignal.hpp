#ifndef XUTILS_X_SIGNAL_HPP
#define XUTILS_X_SIGNAL_HPP 1

#include <csignal>
#include <XHelper/xcallablehelper.hpp>
#include <XGlobal/xclasshelpermacros.hpp>
#include <XMemory/xmemory.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XSignal;
class XSignalPrivate;
using SignalPtr = std::shared_ptr<XSignal>;

class XSignalData {
protected:
    constexpr XSignalData() = default;
public:
    constexpr virtual ~XSignalData() = default;
    XSignal * m_x_ptr_{};
    int m_sig {-1};
    siginfo_t * m_info{};
    void * m_context{};
};

class X_CLASS_EXPORT XSignal final :public std::enable_shared_from_this<XSignal>
    , public XTwoPhaseConstruction<XSignal>
{
    X_DISABLE_COPY_MOVE(XSignal)
    X_DECLARE_PRIVATE(XSignal)
    X_TWO_PHASE_CONSTRUCTION_CLASS
    std::unique_ptr<XSignalData> m_d_ptr_{};

public:
    template<typename Fn,typename... Args>
    constexpr static auto Register(int sig,int flags,Fn &&,Args && ...args) noexcept -> SignalPtr;
    [[nodiscard]] [[maybe_unused]] constexpr int sig() const noexcept
    { return m_d_ptr_->m_sig;  }

#if 0
    [[nodiscard]] [[maybe_unused]] siginfo_t const & siginfo() const & noexcept
    { return *m_d_ptr_->m_info; }
    [[nodiscard]] [[maybe_unused]] ucontext_t * context() const noexcept
    { return static_cast<ucontext_t *>(m_d_ptr_->m_context); }
#endif

    [[maybe_unused]] void unregister();
    ~XSignal();

private:
    XSignal();
    static bool registerHelper(SignalPtr const & );
    bool construct_(int,int) noexcept;
    void setCall_(XCallableHelper::CallablePtr &&) noexcept;
};

template<typename Fn,typename... Args>
constexpr auto XSignal::Register(int const sig,int const flags,Fn && fn,Args && ...args) noexcept ->SignalPtr {
    auto obj{ CreateSharedPtr({},Parameter{sig,flags}) };
    if (!obj) { return {}; }
    auto const d{ obj->m_d_ptr_.get() };
    auto callPtr { XCallableHelper::createCallable(std::forward<Fn>(fn)
        ,std::cref(d->m_sig)
        ,std::cref(d->m_info)
        ,std::cref(d->m_context)
        ,std::forward<Args>(args)...) };
    if (!callPtr) { return {}; }

    obj->setCall_(std::move(callPtr));
    registerHelper(obj);
    return obj;
}

template<typename Fn,typename... Args>
[[maybe_unused]] constexpr auto SignalRegister(int const sig,int const flags,Fn && fn,Args && ...args)
{ return XSignal::Register(sig,flags,std::forward<Fn>(fn),std::forward<Args>(args)...); }

[[maybe_unused]] X_API bool emitSignal(int pid_,int sig_,sigval const &val_) noexcept;

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
