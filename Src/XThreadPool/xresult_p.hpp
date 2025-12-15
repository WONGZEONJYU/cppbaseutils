#ifndef XUTILS2_X_RESULT_P_HPP
#define XUTILS2_X_RESULT_P_HPP 1

#include <XThreadPool/xresult.hpp>
#include <XAtomic/xatomic.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class X_CLASS_EXPORT XResultPrivate final : public XResultData {
    X_DISABLE_COPY_MOVE(XResultPrivate)
public:
    X_DECLARE_PUBLIC(XResult)

    mutable std::promise<std::any> m_result_{};
    mutable XAtomicBool m_recall_{},m_allow_get_{};
#ifdef UNUSE_STD_THREAD_LOCAL
    mutable XThreadLocalConstVoid m_isSelf{};
#endif

    constexpr XResultPrivate() = default;
    ~XResultPrivate() override = default;
    std::any get_value() const override;
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
