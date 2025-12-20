#ifndef XUTILS2_X_SIGNAL_P_HPP
#define XUTILS2_X_SIGNAL_P_HPP 1

#include <Win/XSignal/xsignal.hpp>
#include <unordered_map>
#include <mutex>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class X_CLASS_EXPORT XSignalPrivate : public XSignalData {

public:
    X_DECLARE_PUBLIC(XSignal)

    inline static std::unordered_map<std::string,XSignalPrivate*> sm_signals{};
    std::string m_key{};
    CallablePtr m_callable{};

    static int __stdcall HandlerRoutine(unsigned long);
    explicit XSignalPrivate();
    ~XSignalPrivate() override;
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
