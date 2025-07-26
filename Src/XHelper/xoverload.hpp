#ifndef X_OVERLOAD_HPP
#define X_OVERLOAD_HPP 1

#include <XHelper/xversion.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template <typename... Args>
class XOverload {
public:
    template <typename R, typename C>
    inline constexpr auto operator()(R(C::*ptr)(Args...) ) const noexcept ->decltype(ptr)
    { return ptr; }

    template <typename R, typename C>
    inline constexpr auto operator()(R(C::*ptr)(Args...) &) const noexcept ->decltype(ptr)
    { return ptr; }

    template <typename R, typename C>
    inline constexpr auto operator()(R(C::*ptr)(Args...) &&) const noexcept ->decltype(ptr)
    { return ptr; }

    template <typename R, typename C>
    inline constexpr auto operator()(R(C::*ptr)(Args...) volatile) const noexcept ->decltype(ptr)
    { return ptr; }

    template <typename R, typename C>
    inline constexpr auto operator()(R(C::*ptr)(Args...) volatile &) const noexcept ->decltype(ptr)
    { return ptr; }

    template <typename R, typename C>
    inline constexpr auto operator()(R(C::*ptr)(Args...) volatile &&) const noexcept ->decltype(ptr)
    { return ptr; }

    template <typename R, typename C>
    inline static constexpr auto of(R(C::*ptr)(Args...) ) noexcept -> decltype(ptr)
    { return ptr; }

    template <typename R, typename C>
    inline static constexpr auto of(R(C::*ptr)(Args...) & ) noexcept -> decltype(ptr)
    { return ptr; }

    template <typename R, typename C>
    inline static constexpr auto of(R(C::*ptr)(Args...) && ) noexcept -> decltype(ptr)
    { return ptr; }

    template <typename R, typename C>
    inline static constexpr auto of(R(C::*ptr)(Args...) volatile) noexcept -> decltype(ptr)
    { return ptr; }

    template <typename R, typename C>
    inline static constexpr auto of(R(C::*ptr)(Args...) volatile & ) noexcept -> decltype(ptr)
    { return ptr; }

    template <typename R, typename C>
    inline static constexpr auto of(R(C::*ptr)(Args...) volatile && ) noexcept -> decltype(ptr)
    { return ptr; }

    template <typename R, typename C>
    inline constexpr auto operator()(R (C::*ptr)(Args...) const) const noexcept -> decltype(ptr)
    { return ptr; }

    template <typename R, typename C>
    inline constexpr auto operator()(R (C::*ptr)(Args...) const &) const noexcept -> decltype(ptr)
    { return ptr; }

    template <typename R, typename C>
    inline constexpr auto operator()(R (C::*ptr)(Args...) const &&) const noexcept -> decltype(ptr)
    { return ptr; }

    template <typename R, typename C>
    inline constexpr auto operator()(R (C::*ptr)(Args...) const volatile) const noexcept -> decltype(ptr)
    { return ptr; }

    template <typename R, typename C>
    inline constexpr auto operator()(R (C::*ptr)(Args...) const volatile & ) const noexcept -> decltype(ptr)
    { return ptr; }

    template <typename R, typename C>
    inline constexpr auto operator()(R (C::*ptr)(Args...) const volatile &&) const noexcept -> decltype(ptr)
    { return ptr; }

    template <typename R, typename C>
    inline static constexpr auto of(R (C::*ptr)(Args...) const) noexcept -> decltype(ptr)
    { return ptr; }

    template <typename R, typename C>
    inline static constexpr auto of(R (C::*ptr)(Args...) const &) noexcept -> decltype(ptr)
    { return ptr; }

    template <typename R, typename C>
    inline static constexpr auto of(R (C::*ptr)(Args...) const &&) noexcept -> decltype(ptr)
    { return ptr; }

    template <typename R, typename C>
    inline static constexpr auto of(R (C::*ptr)(Args...) const volatile) noexcept -> decltype(ptr)
    { return ptr; }

    template <typename R, typename C>
    inline static constexpr auto of(R (C::*ptr)(Args...) const volatile &) noexcept -> decltype(ptr)
    { return ptr; }

    template <typename R, typename C>
    inline static constexpr auto of(R (C::*ptr)(Args...) const volatile &&) noexcept -> decltype(ptr)
    { return ptr; }

    template <typename R>
    inline constexpr auto operator()(R (*ptr)(Args...)) const noexcept -> decltype(ptr)
    { return ptr; }

    template <typename R>
    inline static constexpr auto of(R (*ptr)(Args...)) noexcept -> decltype(ptr)
    { return ptr; }
};

template <typename... Args> [[maybe_unused]] inline constexpr XOverload<Args...> xOverload {};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
