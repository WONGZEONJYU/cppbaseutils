#ifndef X_GENERIC_ATOMIC_HPP
#define X_GENERIC_ATOMIC_HPP 1

#include <XHelper/xversion.hpp>
#include <XGlobal/xtypes.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template<int Size> struct XAtomicOpsSupport {
    enum { IsSupported = Size == sizeof(int) || Size == sizeof(xptrdiff) };
};

template <typename T> struct XAtomicAdditiveType {
    using AdditiveT = T;
    [[maybe_unused]] inline static constexpr auto AddScale {1};
};

template <typename T> struct XAtomicAdditiveType<T *> {
    using AdditiveT = xptrdiff;
    [[maybe_unused]] inline static constexpr auto AddScale{sizeof(T)};
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
