#ifndef X_UTILITY_HPP
#define X_UTILITY_HPP 1

#include <XHelper/xversion.hpp>
#include <type_traits>
#include <utility>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

#define HAS_MEM_FUNC(name,...) \
template<typename ,typename = std::void_t<>> \
struct hasMemFunc : std::false_type {}; \
template<typename Class_> \
struct hasMemFunc<Class_, \
std::void_t<decltype(std::declval<Class_>().name(__VA_ARGS__))>> \
: std::true_type {}; \
template<typename Class_> \
static inline constexpr auto hasMemFunc_v = hasMemFunc<Class_>::value;

#define HAS_MEM_VALUE(name) \
template<typename ,typename = std::void_t<>> \
struct hasMemValue : std::false_type {}; \
template<typename Class_> \
struct hasMemValue<Class_,std::void_t<decltype(Class_::name)>> \
: std::true_type {}; \
template<typename Class_> \
static inline constexpr auto hasMemValue_v = hasMemValue<Class_>::value;

#define HAS_MEM_TYPE(name) \
template<typename ,typename = std::void_t<>> \
struct hasMemType : std::false_type {}; \
template<typename Class_> \
struct hasMemType<Class_,std::void_t<typename Class_::name>> \
: std::true_type {}; \
template<typename Class_> \
static inline constexpr auto hasMemType_v = hasMemType<Class_>::value;

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
