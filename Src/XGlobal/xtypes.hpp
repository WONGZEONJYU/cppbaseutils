#ifndef XTYPES_HPP
#define XTYPES_HPP

#include <XGlobal/xprocessordetection.hpp>
#include <XHelper/xversion.hpp>
#include <cstddef>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

using uchar [[maybe_unused]] = unsigned char;
using ushort [[maybe_unused]] = unsigned short;
using uint [[maybe_unused]] = unsigned int ;
using ulong [[maybe_unused]] = unsigned long ;

using xint8 = signed char;         /* 8 bit signed */
using xuint8 = unsigned char;      /* 8 bit unsigned */
using xint16 = short;              /* 16 bit signed */
using xuint16 = unsigned short;    /* 16 bit unsigned */
using xint32 = int;                /* 32 bit signed */
using xuint32 = unsigned int;      /* 32 bit unsigned */
using xint64 = long long;           /* 64 bit signed */
using xuint64 = unsigned long long; /* 64 bit unsigned */

template <int> struct XIntegerForSize;
template <>  struct XIntegerForSize<1> { typedef xuint8  Unsigned; typedef xint8  Signed; };
template <>  struct XIntegerForSize<2> { typedef xuint16 Unsigned; typedef xint16 Signed; };
template <>  struct XIntegerForSize<4> { typedef xuint32 Unsigned; typedef xint32 Signed; };
template <>  struct XIntegerForSize<8> { typedef xuint64 Unsigned; typedef xint64 Signed; };

template <typename T> struct XIntegerForSizeof: XIntegerForSize<sizeof(T)> { };
using xregisterint [[maybe_unused]] = XIntegerForSize<X_PROCESSOR_WORDSIZE>::Signed;
using xregisteruint [[maybe_unused]] = XIntegerForSize<X_PROCESSOR_WORDSIZE>::Unsigned;
using xuintptr [[maybe_unused]] = XIntegerForSizeof<void *>::Unsigned;
using xptrdiff [[maybe_unused]] = XIntegerForSizeof<void *>::Signed;
using xintptr [[maybe_unused]] = xptrdiff;
using xsizetype [[maybe_unused]] = XIntegerForSizeof<std::size_t>::Signed;

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
