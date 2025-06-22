#ifndef X_TYPETRAITS_HPP
#define X_TYPETRAITS_HPP

#include <XHelper/xversion.hpp>
#include <type_traits>
#include <memory>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template<typename>
struct is_smart_pointer : std::false_type {};

// 标准库智能指针
template<typename T>
struct is_smart_pointer<std::unique_ptr<T>> : std::true_type {};

template<typename T, typename D>
struct is_smart_pointer<std::unique_ptr<T, D>> : std::true_type {};

template<typename T>
struct is_smart_pointer<std::shared_ptr<T>> : std::true_type {};

template<typename T>
struct is_smart_pointer<std::weak_ptr<T>> : std::true_type {};

template<typename T>
struct is_smart_pointer<T &> : is_smart_pointer<T> {};

template<typename T>
struct is_smart_pointer<T &&> : is_smart_pointer<T> {};

template<typename T>
struct is_smart_pointer<const T> : is_smart_pointer<T> {};

template<typename T>
struct is_smart_pointer<const T &> : is_smart_pointer<T> {};

template<typename T>
struct is_smart_pointer<const T &&> : is_smart_pointer<T> {};

template<typename T>
struct is_smart_pointer<volatile T> : is_smart_pointer<T> {};

template<typename T>
struct is_smart_pointer<volatile T &> : is_smart_pointer<T> {};

template<typename T>
struct is_smart_pointer<volatile T &&> : is_smart_pointer<T> {};

template<typename T>
struct is_smart_pointer<const volatile T> : is_smart_pointer<T> {};

template<typename T>
struct is_smart_pointer<const volatile T&> : is_smart_pointer<T> {};

template<typename T>
struct is_smart_pointer<const volatile T&&> : is_smart_pointer<T> {};

template<typename T>
inline constexpr bool is_smart_pointer_v = is_smart_pointer<T>::value;

// 获取智能指针的元素类型
template<typename>
struct smart_pointer_element_type {};

template<typename T>
struct smart_pointer_element_type<std::unique_ptr<T>> {
    using type [[maybe_unused]] = T;
};

template<typename T, typename D>
struct smart_pointer_element_type<std::unique_ptr<T, D>> {
    using type [[maybe_unused]] = T;
};

template<typename T>
struct smart_pointer_element_type<std::shared_ptr<T>> {
    using type [[maybe_unused]] = T;
};

template<typename T>
struct smart_pointer_element_type<std::weak_ptr<T>> {
    using type [[maybe_unused]] = T;
};

template<typename T>
using smart_pointer_element_type_t [[maybe_unused]] = typename smart_pointer_element_type<T>::type;

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
