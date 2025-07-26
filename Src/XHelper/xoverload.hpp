#ifndef X_OVERLOAD_HPP
#define X_OVERLOAD_HPP 1

#include <XHelper/xversion.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template <typename... Args>
class XOverload {

#define FOR_EACH_(op) op() op(noexcept)

#define MAKE_OPERATOR_AND_OF1(cvref) \
template <typename R> \
inline constexpr auto operator()(R (*ptr)(Args...) cvref) const noexcept -> decltype(ptr) \
{ return ptr; } \
template <typename R> \
inline static constexpr auto of(R (*ptr)(Args...) cvref) noexcept -> decltype(ptr) \
{ return ptr; }

#define FOR_EACH_CVREF(op) FOR_EACH_(op) \
op(&) op(&&) \
op(volatile) op(volatile &) op(volatile &&)  \
op(const) op(const &) op(const &&) \
op(const volatile) op(const volatile &) op(const volatile &&) \
op(& noexcept) op(&& noexcept) \
op(volatile noexcept) op(volatile & noexcept) op(volatile && noexcept)  \
op(const noexcept) op(const & noexcept) op(const && noexcept) \
op(const volatile noexcept) op(const volatile & noexcept) op(const volatile && noexcept) \

#define MAKE_OPERATOR_AND_OF2(cvref) \
template <typename R, typename C> \
inline constexpr auto operator()(R(C::*ptr)(Args...) cvref) const noexcept ->decltype(ptr) \
{ return ptr; } \
template <typename R, typename C> \
inline static constexpr auto of(R(C::*ptr)(Args...) cvref) noexcept -> decltype(ptr) \
{ return ptr; }

public:
    FOR_EACH_(MAKE_OPERATOR_AND_OF1)
    FOR_EACH_CVREF(MAKE_OPERATOR_AND_OF2)

#undef FOR_EACH_
#undef MAKE_OPERATOR_AND_OF1
#undef FOR_EACH_CVREF
#undef MAKE_OPERATOR_AND_OF2
};

template <typename... Args> [[maybe_unused]] inline constexpr XOverload<Args...> xOverload {};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
