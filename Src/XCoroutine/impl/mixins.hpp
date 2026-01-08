#ifndef XUTILS2_MIXINS_HPP
#define XUTILS2_MIXINS_HPP 1

#pragma once

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    template<typename T, typename Awaiter>
    constexpr auto AwaitTransformMixin::await_transform(T && value)
    { return Awaiter {std::forward<T>(value)}; }

    template<Awaitable T>
    constexpr auto && AwaitTransformMixin::await_transform(T && awaitable)
    { return std::forward<T>(awaitable); }

    template<Awaitable T>
    constexpr auto & AwaitTransformMixin::await_transform(T & awaitable)
    { return awaitable; }

}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
