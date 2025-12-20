#ifndef XUTILS2_XSIGNAL_HPP
#define XUTILS2_XSIGNAL_HPP

#include <windows.h>
#include <XHelper/xcallablehelper.hpp>
#include <XGlobal/xclasshelpermacros.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XSignal;
class XSignalPrivate;

class XSignalData {
public:
    XSignal * m_x_ptr{};
    unsigned long m_sigEvent{};
protected:
    constexpr XSignalData() = default;
public:
    virtual ~XSignalData() = default;
};

class X_CLASS_EXPORT XSignal final {
    X_DECLARE_PRIVATE_D(m_d_ptr_,XSignal)
    std::unique_ptr<XSignalData> m_d_ptr_{};

public:
    explicit XSignal();

    template<typename ...Args>
    explicit XSignal(std::string name,Args && ...args) : XSignal{}
    { registerHandler(std::move(name),std::forward<Args>(args)...); }

    ~XSignal();

    template<typename Fn,typename ...Args>
    void registerHandler(std::string name,Fn && fn,Args && ...args) noexcept {
        if (name.empty()) { return; }
        auto f {
            XCallableHelper::createCallable(std::forward<decltype(fn)>(fn)
            ,std::cref(m_d_ptr_->m_sigEvent),std::forward<Args>(args)...)
        };
        setHandlerHelper(std::move(name),std::move(f));
    }

private:
    void setHandlerHelper(std::string ,CallablePtr) noexcept;
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
