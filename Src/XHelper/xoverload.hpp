#ifndef X_OVERLOAD_HPP
#define X_OVERLOAD_HPP 1

#include <XGlobal/xversion.hpp>
#include <XHelper/xdecorator.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template <typename... Args>
class XOverload {

public:
    #define MAKE_OPERATOR_AND_OF(...) \
        template <typename R> \
        inline constexpr auto operator()(R (*ptr)(Args...) __VA_ARGS__) const noexcept -> decltype(ptr) \
        { return ptr; } \
        template <typename R> \
        inline static constexpr auto of(R (*ptr)(Args...) __VA_ARGS__) noexcept -> decltype(ptr) \
        { return ptr; }

    FOR_EACH_DECORATOR(MAKE_OPERATOR_AND_OF)
    #undef MAKE_OPERATOR_AND_OF

    #define MAKE_OPERATOR_AND_OF(...) \
        template <typename R, typename C> \
        inline constexpr auto operator()(R(C::*ptr)(Args...) __VA_ARGS__) const noexcept ->decltype(ptr) \
        { return ptr; } \
        template <typename R, typename C> \
        inline static constexpr auto of(R(C::*ptr)(Args...) __VA_ARGS__) noexcept -> decltype(ptr) \
        { return ptr; }

    FOR_EACH_CVREF_DECORATOR_NOEXCEPT(MAKE_OPERATOR_AND_OF)
    #undef MAKE_OPERATOR_AND_OF
};

template <typename... Args> [[maybe_unused]] inline constexpr XOverload<Args...> xOverload {};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
