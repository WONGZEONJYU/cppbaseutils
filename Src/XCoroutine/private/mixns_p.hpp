#ifndef XUTILS2_MIXNS_P_HPP
#define XUTILS2_MIXNS_P_HPP 1

#pragma once

#include <XHelper/xversion.hpp>
#include <type_traits>
#include <XCoroutine/private/coroutine_p.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    template<typename T>
    struct awaiter_type;

    template<typename T>
    using awaiter_type_t = awaiter_type<T>::type;

    struct AwaitTransformMixin {

        template<typename T, typename Awaiter = awaiter_type_t<std::remove_cvref_t<T>>>
        static constexpr auto await_transform(T && value);

        //! If the type T is already an awaitable (including Task or LazyTask), then just forward it as it is.
        template<Awaitable T>
        static constexpr auto && await_transform(T && awaitable);

        //! \copydoc template<Awaitable T> QCoro::TaskPromiseBase::await_transform(T &&)
        template<Awaitable T>
        static constexpr auto & await_transform(T &awaitable);
    };

}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#include <XCoroutine/impl/mixins.hpp>

#endif
