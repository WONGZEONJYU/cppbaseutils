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


// 检查成员函数是否是const的辅助模板
template<typename T>
struct [[maybe_unused]] is_const_member_function;

template<typename T, typename... Args>
struct [[maybe_unused]] is_const_member_function<T(Args...) const> : std::true_type {};

template<typename T, typename... Args>
struct [[maybe_unused]] is_const_member_function<T(Args...)> : std::false_type {};

#if 1
// 检查派生类是否正确重写了基类的虚函数
template<typename Base, typename Derived>
class [[maybe_unused]] virtual_override_checker final {

    static_assert(std::is_base_of_v<Base,Derived>,"err!");

private:
    // 检查func函数的const属性
    template<typename T>
    [[maybe_unused]] static auto check_func_const(int)
    -> decltype(std::declval<const T>().run(), std::true_type{}){
        return {};
    };

    template<typename>
    static std::false_type check_func_const(...){return {};};

public:

    static constexpr bool base_func_is_const {decltype(check_func_const<Base>(0))::value};

    static constexpr bool derived_func_is_const {decltype(check_func_const<Derived>(0))::value};

    [[maybe_unused]] static constexpr bool is_correctly_overridden {base_func_is_const == derived_func_is_const};
};
#endif

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
