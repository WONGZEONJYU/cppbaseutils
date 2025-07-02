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

template<std::size_t... Ints>
struct index_Sequence {
    using type = index_Sequence;
    static inline constexpr auto size() noexcept {return Size;}
    static inline constexpr auto Size {sizeof...(Ints)};
};

namespace forward {

    template<std::size_t N,std::size_t... Ints>
    struct make_index_sequence_impl : make_index_sequence_impl<N-1,N-1,Ints...> {};

    template<std::size_t... Ints>
    struct make_index_sequence_impl<0,Ints...> : index_Sequence<Ints...> {};

    template<std::size_t N>
    using make_index_sequence = typename make_index_sequence_impl<N>::type;

    template<typename... T>
    using index_sequence_for = make_index_sequence<sizeof...(T)>;

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
    struct make_reverse_index_sequence_impl<0,Ints...> : index_Sequence<Ints...> {};

    template<std::size_t N>
    using make_reverse_index_sequence = typename make_reverse_index_sequence_impl<N>::type;

    template<typename... T>
    using reverse_index_sequence_for = make_reverse_index_sequence<sizeof...(T)>;
#ifdef XDOC
    make_reverse_index_sequence<5>:make_reverse_index_sequence<4,4>
    make_reverse_index_sequence<4,4> : make_reverse_index_sequence<3,4,3>
    make_reverse_index_sequence<3,4,3> : make_reverse_index_sequence<2,4,3,2>
    make_reverse_index_sequence<2,4,3,2>: make_reverse_index_sequence<1,4,3,2,1>
    make_reverse_index_sequence<1,4,3,2,1>:make_reverse_index_sequence<0,4,3,2,1,0>
    make_reverse_index_sequence<0,4,3,2,1,0>:index_sequence<4,3,2,1,0>
#endif
}

template <typename... Ts> struct List{
    static constexpr auto size{sizeof...(Ts)};
};

template<typename> struct [[maybe_unused]] SizeOfList {
    static constexpr size_t value {1};
};

template<> struct [[maybe_unused]] SizeOfList<List<>> {
    static constexpr size_t value{};
};

template<typename ...Ts> struct [[maybe_unused]] SizeOfList<List<Ts...>> {
    static constexpr auto value {List<Ts...>::size};
};

template <typename Head, typename... Tail>
struct List<Head, Tail...> {
    static constexpr size_t size {1 + sizeof...(Tail)};
    using Car [[maybe_unused]] = Head;
    using Cdr [[maybe_unused]] = List<Tail...>;
};

template <typename, typename> struct List_Append;

template <typename... L1, typename...L2> struct List_Append<List<L1...>, List<L2...>> {
    using Value [[maybe_unused]] = List<L1..., L2...>;
};

template <typename L, int N> class [[maybe_unused]] List_Left {
    using List_Car = List<typename L::Car>;
    using List_Left_ = List_Left<typename L::Cdr, N - 1>;
    using List_Left_V = typename List_Left_::Value;
public:
    using Value = typename List_Append<List_Car,List_Left_V>::Value;
};

template <typename L> struct [[maybe_unused]] List_Left<L, 0>{
    using Value = List<>;
};

/*
 * using List_t = List<int8_t,uint8_t,int16_t,uint16_t,int32_t,uint32_t,int64_t,uint64_t,float,double> //size = 10
 * List_Left<List_t,List_t::size>{
 *     using List_Car = List<int8_t>;
 *     using List_Left_ = List_Left<uint8_t,int16_t,uint16_t,int32_t,uint32_t,int64_t,uint64_t,float,double,9>;
 *     using List_Left_V =
 *     using Value = List_Append<>
 *
 * }
 *
 *
 *
 * */

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
