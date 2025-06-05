#ifndef XTYPES_HPP
#define XTYPES_HPP

#include <XGlobal/xprocessordetection.hpp>
#include <XHelper/xhelper.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

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
using xregisterint = XIntegerForSize<X_PROCESSOR_WORDSIZE>::Signed;
using xregisteruint = XIntegerForSize<X_PROCESSOR_WORDSIZE>::Unsigned;
using xuintptr = XIntegerForSizeof<void *>::Unsigned;
using xptrdiff = XIntegerForSizeof<void *>::Signed;
//typedef XIntegerForSizeof<void *>::Signed xptrdiff;
using xintptr = xptrdiff;
using xsizetype = XIntegerForSizeof<std::size_t>::Signed;

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
