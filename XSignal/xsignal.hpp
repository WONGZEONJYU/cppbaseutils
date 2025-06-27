#ifndef X_SIGNAL_HPP
#define X_SIGNAL_HPP 1

#include "xabstractsignal.hpp"

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XSignal;
using Signal_Ptr = std::shared_ptr<XSignal>;

class XSignal : public XAbstractSignal {
    X_DISABLE_COPY_MOVE(XSignal)
    static Signal_Ptr create(const int &,const int &);
protected:
    XSignal() = default;
public:
    template<typename... Args>
    inline static auto Register(const int &sig,const int &flags,Args&& ...args){
        const auto obj{create(sig,flags)};
        if (obj){
            obj->Callable_join(std::forward<Args>(args)...);
        }
        return obj;
    }

    [[maybe_unused]] static bool Send_signal(const int &pid_,const int &sig_,const sigval &val_);
    [[maybe_unused]] static siginfo_t siginfo(const int&sig);

    static void Unregister(const int &sig);
    ~XSignal() override = default;
};

template<typename... Args>
[[maybe_unused]] static inline auto Signal_Register(const int &sig,const int &flags,Args&& ...args){
    return XSignal::Register(sig,flags,std::forward<Args>(args)...);
}

[[maybe_unused]] static inline void Signal_Unregister(const int &sig){
    XSignal::Unregister(sig);
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
