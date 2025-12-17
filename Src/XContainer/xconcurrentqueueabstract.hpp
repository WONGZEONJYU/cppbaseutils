#ifndef XUTILS2_X_CONCURRENT_QUEUE_ABSTRACT_HPP
#define XUTILS2_X_CONCURRENT_QUEUE_ABSTRACT_HPP 1

#include <cstddef>              // for max_align_t
#include <cstdlib>
#include <type_traits>
#include <algorithm>
#include <utility>
#include <limits>
#include <climits>		// for CHAR_BIT
#include <array>
#include <thread>		// partly for __WINPTHREADS_VERSION if on MinGW-w64 w/ POSIX threading
#include <mutex>        // used for thread exit synchronization
#include <XHelper/xversion.hpp>
#include <XGlobal/xclasshelpermacros.hpp>
#include <XAtomic/xatomic.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)


XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
