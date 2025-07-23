#ifndef X_TYPETRAITS_HPP
#define X_TYPETRAITS_HPP

#include <XHelper/xversion.hpp>
#include <type_traits>
#include <memory>
#include <tuple>
#ifdef HAS_BOOST
#include <boost/type_index.hpp>
#endif

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
inline constexpr auto is_smart_pointer_v = is_smart_pointer<T>::value;

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

// 检查成员函数是否是const的辅助模板
template<typename>
struct [[maybe_unused]] is_const_member_function;

template<typename Fn, typename... Args>
struct [[maybe_unused]] is_const_member_function<Fn(Args...) const> : std::true_type {};

template<typename Fn, typename... Args>
struct [[maybe_unused]] is_const_member_function<Fn(Args...) const noexcept> : std::true_type {};

template<typename Fn, typename... Args>
struct [[maybe_unused]] is_const_member_function<Fn(Args...) const volatile> : std::true_type {};

template<typename Fn, typename... Args>
struct [[maybe_unused]] is_const_member_function<Fn(Args...) const volatile noexcept> : std::true_type {};

template<typename Fn, typename... Args>
struct [[maybe_unused]] is_const_member_function<Fn(Args...) volatile> : std::false_type {};

template<typename Fn, typename... Args>
struct [[maybe_unused]] is_const_member_function<Fn(Args...) volatile noexcept> : std::false_type {};

template<typename Fn, typename... Args>
struct [[maybe_unused]] is_const_member_function<Fn(Args...) noexcept> : std::false_type {};

template<typename Fn, typename... Args>
struct [[maybe_unused]] is_const_member_function<Fn(Args...)> : std::false_type {};

template<typename... Args>
inline constexpr auto is_const_member_function_v [[maybe_unused]] {is_const_member_function<Args...>::value};

template<typename>
struct [[maybe_unused]] is_tuple : std::false_type {};

template<typename ...Args>
struct [[maybe_unused]] is_tuple<std::tuple<Args...>> : std::true_type {};

template<typename Tuple_>
struct [[maybe_unused]] is_tuple<const Tuple_> : is_tuple<Tuple_> {};

template<typename Tuple_>
struct [[maybe_unused]] is_tuple<Tuple_ &> : is_tuple<Tuple_> {};

template<typename Tuple_>
struct [[maybe_unused]] is_tuple<Tuple_ &&> : is_tuple<Tuple_> {};

template<typename Tuple_>
struct [[maybe_unused]] is_tuple<const Tuple_ &> : is_tuple<Tuple_> {};

template<typename Tuple_>
struct [[maybe_unused]] is_tuple<const Tuple_ &&> : is_tuple<Tuple_> {};

template<typename Tuple_>
struct [[maybe_unused]] is_tuple<const volatile Tuple_ &> : is_tuple<Tuple_> {};

template<typename Tuple_>
struct [[maybe_unused]] is_tuple<const volatile Tuple_ &&> : is_tuple<Tuple_> {};

template<typename Tuple_>
struct [[maybe_unused]] is_tuple<volatile Tuple_ > : is_tuple<Tuple_> {};

template<typename Tuple_>
struct [[maybe_unused]] is_tuple<volatile Tuple_ &> : is_tuple<Tuple_> {};

template<typename Tuple_>
struct [[maybe_unused]] is_tuple<volatile Tuple_ &&> : is_tuple<Tuple_> {};

template<typename Tuple_>
[[maybe_unused]] inline constexpr auto is_tuple_v {is_tuple<Tuple_>::value};

template<typename Ty>
[[maybe_unused]] X_API inline auto typeName(Ty){
#ifdef HAS_BOOST
    return boost::typeindex::type_id_with_cvr<Ty>().pretty_name();
#else
    return typeid(Ty).name();
#endif
}

template<typename Ty>
[[maybe_unused]] X_API inline auto typeName(){
#ifdef HAS_BOOST
    return boost::typeindex::type_id_with_cvr<Ty>().pretty_name();
#else
    return typeid(Ty).name();
#endif
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
