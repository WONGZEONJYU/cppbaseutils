#include "xatomic.hpp"

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

// static checks

// The following specializations must always be defined
static_assert(sizeof(XAtomicInteger<unsigned>));
static_assert(sizeof(XAtomicInteger<long>));
static_assert(sizeof(XAtomicInteger<unsigned long>));
static_assert(sizeof(XAtomicInteger<xuintptr>));
static_assert(sizeof(XAtomicInteger<xptrdiff>));
static_assert(sizeof(XAtomicInteger<char32_t>));

static_assert(sizeof(XAtomicInteger<short>));
static_assert(sizeof(XAtomicInteger<unsigned short>));
static_assert(sizeof(XAtomicInteger<wchar_t>));
static_assert(sizeof(XAtomicInteger<char16_t>));

static_assert(sizeof(XAtomicInteger<char>));
static_assert(sizeof(XAtomicInteger<unsigned char>));
static_assert(sizeof(XAtomicInteger<signed char>));
static_assert(sizeof(XAtomicInteger<bool>));


#ifdef X_ATOMIC_INT64_IS_SUPPORTED
static_assert(sizeof(XAtomicInteger<xint64>));
static_assert(sizeof(XAtomicInteger<xuint64>));
#endif

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
