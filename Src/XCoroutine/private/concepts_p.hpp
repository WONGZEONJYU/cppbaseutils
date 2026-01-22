#ifndef XUTILS2_CONCEPTS_P_HPP
#define XUTILS2_CONCEPTS_P_HPP

#ifndef X_COROUTINE_
#error Do not concepts_p.hpp directly
#endif

#pragma once

#include <concepts>
#include <XGlobal/xversion.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace concepts {
    template<typename T>
    concept destructible = std::is_nothrow_destructible_v<T>;

    template<typename T, typename ... Args>
    concept constructible_from = destructible<T>
        && std::is_constructible_v<T, Args...>;
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
