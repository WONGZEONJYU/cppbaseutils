#ifndef X_UTILITY_HPP
#define X_UTILITY_HPP 1

#include <XGlobal/xversion.hpp>
#include <type_traits>
#include <utility>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

#define HAS_MEM_FUNC(name,...) \
    template<typename ,typename = std::void_t<>> \
    struct hasMemFunc_##name : std::false_type {}; \
    template<typename Class_> \
    struct hasMemFunc_##name<Class_, \
    std::void_t<decltype(std::declval<Class_>().name(__VA_ARGS__))>> : std::true_type {}; \
    template<typename Class_> \
    inline constexpr auto hasMemFunc_##name##_v { hasMemFunc_##name<Class_>::value };

#define HAS_MEM_VALUE(name) \
    template<typename ,typename = std::void_t<>> \
    struct hasMemValue_##name : std::false_type {}; \
    template<typename Class_> \
    struct hasMemValue_##name<Class_,std::void_t<decltype(Class_::name)>> : std::true_type {}; \
    template<typename Class_> \
    inline constexpr auto hasMemValue_##name##_v { hasMemValue_##name <Class_>::value };

#define HAS_MEM_TYPE(name) \
    template<typename ,typename = std::void_t<>> \
    struct hasMemType_##name : std::false_type {}; \
    template<typename Class_> \
    struct hasMemType_##name<Class_,std::void_t<typename Class_::name>> : std::true_type {}; \
    template<typename Class_> \
    inline constexpr auto hasMemType_v { hasMemType_##name<Class_>::value };

template<std::size_t... Ints>
struct index_Sequence {
    using type = index_Sequence;
    static constexpr auto Size {sizeof...(Ints)};
    static constexpr auto size() noexcept { return Size; }
};

namespace forward {

    template<std::size_t N,std::size_t... Ints>
    struct make_index_sequence_impl : make_index_sequence_impl<N-1,N-1,Ints...> {};

    template<std::size_t... Ints>
    struct make_index_sequence_impl<0,Ints...> final : index_Sequence<Ints...> {};

    template<std::size_t N>
    using make_Index_Sequence = make_index_sequence_impl<N>::type;

    template<typename... T>
    using index_Sequence_for [[maybe_unused]] = make_Index_Sequence<sizeof...(T)>;

#ifdef XDOC
    make_index_sequence_impl<5> : make_index_sequence_impl<4,4>
    make_index_sequence_impl<4,4> : make_index_sequence_impl<3,3,4>
    make_index_sequence_impl<3,3,4> : make_index_sequence_impl<2,2,3,4>
    make_index_sequence_impl<2,2,3,4> : make_index_sequence_impl<1,1,2,3,4>
    make_index_sequence_impl<1,1,2,3,4> : make_index_sequence_impl<0,0,1,2,3,4>
    make_index_sequence_impl<0,0,1,2,3,4> : index_sequence<0,1,2,3,4>
#endif

}

namespace reverse {

    template<std::size_t N, std::size_t... Ints>
    struct make_reverse_index_sequence_impl : make_reverse_index_sequence_impl<N-1,Ints...,N-1> {};

    template<std::size_t... Ints>
    struct make_reverse_index_sequence_impl<0,Ints...> final : index_Sequence<Ints...> {};

    template<std::size_t N>
    using make_reverse_index_sequence = make_reverse_index_sequence_impl<N>::type;

    template<typename... T>
    using reverse_index_sequence_for [[maybe_unused]] = make_reverse_index_sequence<sizeof...(T)>;

#ifdef XDOC
    make_reverse_index_sequence<5>:make_reverse_index_sequence<4,4>
    make_reverse_index_sequence<4,4> : make_reverse_index_sequence<3,4,3>
    make_reverse_index_sequence<3,4,3> : make_reverse_index_sequence<2,4,3,2>
    make_reverse_index_sequence<2,4,3,2>: make_reverse_index_sequence<1,4,3,2,1>
    make_reverse_index_sequence<1,4,3,2,1>:make_reverse_index_sequence<0,4,3,2,1,0>
    make_reverse_index_sequence<0,4,3,2,1,0>:index_sequence<4,3,2,1,0>
#endif
}

// 计算多维数组中单个元素的总数
template<typename T>
constexpr std::size_t calculate_total_elements() noexcept {
    if constexpr (std::is_array_v<T>) {
        constexpr auto current_extent{std::extent_v<T, 0>};
        if constexpr (!current_extent) {
            return calculate_total_elements<std::remove_extent_t<T>>();
        } else {
            return current_extent * calculate_total_elements<std::remove_extent_t<T>>();
        }
    } else {
        return 1;
    }
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
