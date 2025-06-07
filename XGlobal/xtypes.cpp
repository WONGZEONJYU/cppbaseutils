#include "xtypes.hpp"
#include <limits>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

// Statically check assumptions about the environment we're running
// in. The idea here is to error or warn if otherwise implicit Qt
// assumptions are not fulfilled on new hardware or compilers
// (if this list becomes too long, consider factoring into a separate file)
static_assert(UCHAR_MAX == 255, "X assumes that char is 8 bits");
static_assert(sizeof(int) == 4, "X assumes that int is 32 bits");
static_assert(X_POINTER_SIZE == sizeof(void *), "X_POINTER_SIZE defined incorrectly");
static_assert(sizeof(float) == 4, "X assumes that float is 32 bits");
static_assert(sizeof(char16_t) == 2, "X assumes that char16_t is 16 bits");
static_assert(sizeof(char32_t) == 4, "X assumes that char32_t is 32 bits");

static_assert(std::numeric_limits<int>::radix == 2,
                  "X assumes binary integers");
static_assert(std::numeric_limits<int>::max() + std::numeric_limits<int>::lowest() == -1,
                  "X assumes two's complement integers");
static_assert(sizeof(wchar_t) == sizeof(char32_t) || sizeof(wchar_t) == sizeof(char16_t),
              "X assumes wchar_t is compatible with either char32_t or char16_t");

#if !defined(Q_CC_GHS)
static_assert(std::numeric_limits<float>::is_iec559,
                  "X assumes IEEE 754 floating point");
#endif

// Technically, presence of NaN and infinities are implied from the above check,
// but double checking our environment doesn't hurt...
static_assert(std::numeric_limits<float>::has_infinity &&
                  std::numeric_limits<float>::has_quiet_NaN,
                  "X assumes IEEE 754 floating point");

// is_iec559 checks for ISO/IEC/IEEE 60559:2011 (aka IEEE 754-2008) compliance,
// but that allows for a non-binary radix. We need to recheck that.
// Note how __STDC_IEC_559__ would instead check for IEC 60559:1989, aka
// ANSI/IEEE 754−1985, which specifically implies binary floating point numbers.
static_assert(std::numeric_limits<float>::radix == 2,
                  "X assumes binary IEEE 754 floating point");

// not required by the definition of size_t, but we depend on this
static_assert(sizeof(size_t) == sizeof(void *), "size_t and a pointer don't have the same size");
static_assert(sizeof(size_t) == sizeof(xsizetype)); // implied by the definition
static_assert(std::is_same_v<xsizetype, xptrdiff>);
static_assert(std::is_same_v<std::size_t, size_t>);

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
