#ifndef XUTILS2_X_ABSTRACT_RUNNABLE_P_HPP
#define XUTILS2_X_ABSTRACT_RUNNABLE_P_HPP 1

#include <XThreadPool/xabstractrunnable.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class X_CLASS_EXPORT XAbstractRunnablePrivate final : public XAbstractRunnableData {

public:
    X_DECLARE_PUBLIC(XAbstractRunnable)
#ifdef UNUSE_STD_THREAD_LOCAL
    [[maybe_unused]] mutable XThreadLocalConstVoid m_isSelf{};
#endif

    using FuncVer = XAbstractRunnable::FuncVer;
    mutable FuncVer m_is_OverrideConst{};
    mutable XAtomicBool m_recall{};
    mutable std::function<bool()> m_is_running{};
    mutable XAtomicPointer<const void> m_owner{};

    constexpr XAbstractRunnablePrivate() = default;
    ~XAbstractRunnablePrivate() override = default;
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
