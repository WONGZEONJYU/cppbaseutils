#ifndef XUTILS_X_SIGNAL_HPP
#define XUTILS_X_SIGNAL_HPP 1

#include <csignal>
#include <XHelper/xcallablehelper.hpp>
#include <XHelper/xclasshelpermacros.hpp>
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
    virtual ~XSignalData() = default;
    XSignal * m_x_ptr_{};
};

class X_CLASS_EXPORT XSignal final : public XCallableHelper
    ,XTwoPhaseConstruction<XSignal>
{
    X_DISABLE_COPY_MOVE(XSignal)
    X_DECLARE_PRIVATE(XSignal)
    X_TWO_PHASE_CONSTRUCTION_CLASS
    std::unique_ptr<XSignalData> m_d_ptr_{};

public:
    template<typename... Args>
    constexpr static auto Register(int sig,int flags,Args && ...args) noexcept -> SignalPtr;
    [[nodiscard]] [[maybe_unused]] int sig() const noexcept;
    [[nodiscard]] [[maybe_unused]] siginfo_t const & siginfo() const & noexcept;
    [[nodiscard]] [[maybe_unused]] ucontext_t * context() const noexcept;
    [[maybe_unused]] void unregister();
    [[maybe_unused]] static siginfo_t siginfo(int sig);
    static void unregister(int sig);
    ~XSignal() override;

private:
    XSignal();
    static SignalPtr createXSignal(int sig, int flags) noexcept;
    bool construct_(int,int) noexcept;
    void setCall_(CallablePtr &&) noexcept;
};

template<typename... Args>
constexpr auto XSignal::Register(int const sig,int const flags,Args && ...args) noexcept ->SignalPtr {
    auto call { XFactoryInvoker::create(std::forward<Args>(args)...) };
    auto callPtr { XFactoryCallable::create(std::forward<decltype(call)>(call)) };
    if (!callPtr) { return {}; }
    auto ptr { createXSignal(sig,flags) };
    if (!ptr) { return {}; }
    ptr->setCall_(std::forward<decltype(callPtr)>(callPtr));
    return ptr;
}

template<typename... Args>
[[maybe_unused]] constexpr auto SignalRegister(int const sig,int const flags,Args && ...args)
{ return XSignal::Register(sig,flags,std::forward<Args>(args)...); }

[[maybe_unused]] X_API void SignalUnregister(int sig);

[[maybe_unused]] X_API bool emitSignal(int pid_,int sig_,sigval const &val_) noexcept;

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
