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
    static inline constexpr auto Size {sizeof...(Ints)};
    static inline constexpr auto size() noexcept {return Size;}
};

namespace forward {

    template<std::size_t N,std::size_t... Ints>
    struct make_index_sequence_impl : make_index_sequence_impl<N-1,N-1,Ints...> {};

    template<std::size_t... Ints>
    struct make_index_sequence_impl<0,Ints...> final : index_Sequence<Ints...> {};

    template<std::size_t N>
    using make_Index_Sequence = typename make_index_sequence_impl<N>::type;

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
    using make_reverse_index_sequence = typename make_reverse_index_sequence_impl<N>::type;

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

class XObject;

namespace XPrivate {

    namespace detail {
#define FOR_EACH_CVREF(op) op(&) op(const &) op(&&) op(const &&)

        template<typename Object, typename = void>
        class [[maybe_unused]] StorageByValue {
        public:
            Object o;
#define MAKE_GETTER(cvRef) \
            constexpr Object cvRef object() cvRef noexcept \
            { return static_cast<Object cvRef>(o); }
            FOR_EACH_CVREF(MAKE_GETTER)
#undef MAKE_GETTER
        };

        template <typename Object, typename = void>
        class [[maybe_unused]] StorageEmptyBaseClassOptimization : Object {
        public:
            StorageEmptyBaseClassOptimization() = default;
            explicit StorageEmptyBaseClassOptimization(Object &&o) noexcept : Object(std::move(o)) {}
            explicit StorageEmptyBaseClassOptimization(const Object &o) : Object(o) {}
#define MAKE_GETTER(cvRef) \
            constexpr Object cvRef object() cvRef noexcept \
            { return static_cast<Object cvRef>(*this); }
            FOR_EACH_CVREF(MAKE_GETTER)
#undef MAKE_GETTER
        };
#undef FOR_EACH_CVREF
    } //namespace detail

    template <typename Object, typename Tag = void>
    using CompactStorage [[maybe_unused]] = /*typename*/ std::conditional_t<
            std::conjunction_v<std::is_empty<Object>, //逻辑& and
            std::negation<std::is_final<Object>> //逻辑! not
            >,
            detail::StorageEmptyBaseClassOptimization<Object, Tag>,
            detail::StorageByValue<Object, Tag>
        >;

    template <typename T> struct [[maybe_unused]] RemoveRef final { using Type = T; };
    template <typename T> struct [[maybe_unused]] RemoveRef<T&> final { using Type = T; };
    template <typename T> struct [[maybe_unused]] RemoveRef<T&&> final { using Type = T; };
    template <typename T> using RemoveRef_T [[maybe_unused]] = typename RemoveRef<T>::Type;

    template <typename T> struct RemoveConstRef final { using Type = T; };
    template <typename T> struct RemoveConstRef<const T&> final { using Type = T; };
    template <typename T> struct RemoveConstRef<const T&&> final { using Type = T; };
    template <typename T> using RemoveConstRef_T = typename RemoveConstRef<T>::Type;

    template<typename... Ts>
    struct List final {
        static inline constexpr auto size {sizeof...(Ts)};
    };

    template<typename>
    struct [[maybe_unused]] SizeOfList final {
        static inline constexpr std::size_t value{1};
    };

    template<>
    struct [[maybe_unused]] SizeOfList<List<>> final {
        static constexpr std::size_t value{};
    };

    template<typename ...Ts>
    struct [[maybe_unused]] SizeOfList<List<Ts...>> final {
        static inline constexpr auto value{List<Ts...>::size};
    };

    template<typename Head, typename... Tail>
    struct List<Head, Tail...> final {
        static inline constexpr std::size_t size{1 + sizeof...(Tail)};
        using Car [[maybe_unused]] = Head;
        using Cdr [[maybe_unused]] = List<Tail...>;
    };

    template<typename, typename>
    struct List_Append;

    template<typename... L1, typename...L2>
    struct List_Append<List<L1...>, List<L2...>> final {
        using Value [[maybe_unused]] = List<L1..., L2...>;
    };

    template<typename L, std::size_t N>
    struct [[maybe_unused]] List_Left final {
    private:
        using List_Car = List<typename L::Car>;
        using List_Left_R = List_Left<typename L::Cdr, N - 1>;
        using List_Left_Value = typename List_Left_R::Value;
    public:
        using Value = typename List_Append<List_Car, List_Left_Value>::Value;
    };

    template<typename L>
    struct [[maybe_unused]] List_Left<L, 0> final {
        using Value = List<>;
    };

    template<typename L,std::size_t N>
    using List_Left_V = typename List_Left<L,N>::Value;

#ifdef XDOC
            using List_t = List<int8_t,uint8_t,int16_t,uint16_t,int32_t,uint32_t>
            List_Left<List<int8_t,uint8_t,int16_t,uint16_t,int32_t,uint32_t>,List_t::size = 6> = {

                using List_Car1 == List<int8_t>;
                using List_Left_1 == List_Left<List<uint8_t,int16_t,uint16_t,int32_t,uint32_t>,5> == {

                     using List_Car2 == List<uint8_t>;
                     using List_Left_2 == List_Left<List<int16_t,uint16_t,int32_t,uint32_t>,4> == {

                         using List_Car3 == List<int16_t>;
                         using List_Left_3 == List_Left<List<uint16_t,int32_t,uint32_t>,3> == {

                             using List_Car4 == List<uint16_t>;
                             using List_Left_4 = List_Left<List<int32_t,uint32_t>,2> == {

                                   using List_Car5 == List<int32_t>;
                                   using List_Left_5 = List_Left<List<uint32_t>,1> == {

                                         using List_Car6 == List<uint32_t>;
                                         using List_Left_6 == List_Left<List<>,0> == { /*递归终止*/
                                             using Value7 = List<>;
                                         };
                                         using List_Left_V6 == List_Left_6::Value == Value7 == List<>;
                                         using Value6 == List_Append<uint32_t,List_Left_V6>::Value == List_Append<uint32_t,List<>>::Value == List<uint32_t>
                                         /*using Value6 == List<uint32_t>;*/
                                   };
                                   using List_Left_V5 = List_Left_5::Value == Value6 == List<uint32_t>;
                                   using Value5 == List_Append<List<int32_t>,List_Left_V5>::Value == List<int32_t,uint32_t>;
                                   /*using Value5 == List<int32_t,uint32_t>;*/
                             };
                             using List_Left_V4 = List_Left_4::Value == Value5 == List<int32_t,uint32_t>;
                             using Value4 == List_Append<List<uint16_t>,List_Left_V4>::Value == List<uint16_t,int32_t,uint32_t>;
                             /*using Value4 == List<uint16_t,int32_t,uint32_t>;*/
                         };
                         using List_Left_V3 == List_Left_3::Value == Value4 == List<uint16_t,int32_t,uint32_t>;
                         using Value3 == List_Append<List<int16_t>,List_Left_V3>::Value == List<int16_t,uint16_t,int32_t,uint32_t>;
                         /*using Value3 == List<int16_t,uint16_t,int32_t,uint32_t>;*/
                     };
                     using List_Left_V2 == List_Left_2::Value == Value3 == List<int16_t,uint16_t,int32_t,uint32_t>;
                     using Value2 == List_Append<List<uint8_t>,List_Left_V2>::Value == List<uint8_t,int16_t,uint16_t,int32_t,uint32_t>;
                     /*using Value2 == List<uint8_t,int16_t,uint16_t,int32_t,uint32_t>;*/
                };
                using List_Left_V1 == List_Left_1::Value == Value2 == List<uint8_t,int16_t,uint16_t,int32_t,uint32_t>;
                using Value1 == List_Append<List<int8_t>,List_Left_V1>::Value == List_Append<List<int8_t>,List<uint8_t,int16_t,uint16_t,int32_t,uint32_t>>::Value == List<int8_t,uint8_t,int16_t,uint16_t,int32_t,uint32_t>;
                /*using Value1 == List<int8_t,uint8_t,int16_t,uint16_t,int32_t,uint32_t>;*/
            };
#endif

    struct FunctorCallBase {

        template<typename R, typename Lambda>
        [[maybe_unused]] static inline void call_internal([[maybe_unused]] void ** const args, Lambda &&fn)
        noexcept(std::is_nothrow_invocable_v<Lambda>) {
            if constexpr (std::is_void_v<R> || std::is_void_v<std::invoke_result_t<Lambda>>) {
                std::forward<Lambda>(fn)();
            } else {
                if (args[0]){
                    *static_cast<R *>(args[0]) = std::forward<Lambda>(fn)();
                    //*reinterpret_cast<R *>(args[0]) = std::forward<Lambda>(fn)();
                }else {
                    (void)std::forward<Lambda>(fn)();
                }
            }
        }
    };

    template<typename>
    struct [[maybe_unused]] FunctionPointer final {
        enum {
            ArgumentCount [[maybe_unused]] = -1,
            IsPointerToMemberFunction [[maybe_unused]] = false
        };
    };

    template<typename,typename,typename,typename>
    struct FunctorCall;

    template<std::size_t... II, typename... SignalArgs, typename R, typename Function>
    struct FunctorCall<std::index_sequence<II...>, List<SignalArgs...>, R, Function> final : FunctorCallBase {

        [[maybe_unused]] static void call([[maybe_unused]] Function &f, void ** const arg) {
            call_internal<R>(arg, [&] {
                return f((*reinterpret_cast<std::remove_reference_t<SignalArgs> *>(arg[II + 1]))...);
            });
        }
    };

    template<std::size_t... II, typename... SignalArgs, typename R, typename... SlotArgs, typename SlotRet, typename Obj>
    struct FunctorCall<std::index_sequence<II...>, List<SignalArgs...>, R, SlotRet (Obj::*)(SlotArgs...)> final
            : FunctorCallBase {
    private:
        using Function = SlotRet (Obj::*)(SlotArgs...);
    public:
        [[maybe_unused]] static void call([[maybe_unused]] Function f, Obj * const o, void ** const arg) {
            call_internal<R>(arg, [&] {
                return (o->*f)(
                        (*reinterpret_cast<std::remove_reference_t<SignalArgs> *>(arg[II + 1]))...);
            });
        }
    };

    template<std::size_t... II, typename... SignalArgs, typename R, typename... SlotArgs, typename SlotRet, typename Obj>
    struct FunctorCall<std::index_sequence<II...>, List<SignalArgs...>, R, SlotRet (Obj::*)(SlotArgs...) const> final
            : FunctorCallBase {
    private:
        using Function = SlotRet (Obj::*)(SlotArgs...) const;
    public:
        [[maybe_unused]] static void call([[maybe_unused]] Function f, Obj * const o, void ** const arg) {
            call_internal<R>(arg, [&]{
                return (o->*f)(
                        (*reinterpret_cast<std::remove_reference_t<SignalArgs> *>(arg[II + 1]))...);
            });
        }
    };

    template<std::size_t... II, typename... SignalArgs, typename R, typename... SlotArgs, typename SlotRet, typename Obj>
    struct FunctorCall<std::index_sequence<II...>, List<SignalArgs...>, R, SlotRet (Obj::*)(
            SlotArgs...) noexcept> final : FunctorCallBase {
    private:
        using Function = SlotRet (Obj::*)(SlotArgs...) noexcept;
    public:
        [[maybe_unused]] static void call([[maybe_unused]] Function f, Obj * const o, void ** const arg) {

            call_internal<R>(arg, [&]() noexcept {
                return (o->*f)(
                        (*reinterpret_cast<std::remove_reference_t<SignalArgs> *>(arg[II + 1]))...);
            });
        }
    };

    template<std::size_t... II, typename... SignalArgs, typename R, typename... SlotArgs, typename SlotRet, typename Obj>
    struct FunctorCall<std::index_sequence<II...>, List<SignalArgs...>, R, SlotRet (Obj::*)(
            SlotArgs...) const noexcept> final : FunctorCallBase {
    private:
        using Function = SlotRet (Obj::*)(SlotArgs...) const noexcept;
    public:
        [[maybe_unused]] static void call([[maybe_unused]]Function f, [[maybe_unused]] Obj * const o, void ** const arg) {

            call_internal<R>(arg, [&]() noexcept {
                return (o->*f)(
                        (*reinterpret_cast<std::remove_reference_t<SignalArgs> *>(arg[II + 1]))...);
            });
        }
    };

    template<typename Obj, typename Ret, typename... Args>
    struct [[maybe_unused]] FunctionPointer<Ret (Obj::*)(Args...)> final {
        using Object [[maybe_unused]] = Obj;
        using Arguments [[maybe_unused]]  [[maybe_unused]] = List<Args...> ;
        using ReturnType [[maybe_unused]]  [[maybe_unused]] = Ret ;
        using Function = Ret(Obj::*)(Args...);

        enum {
            ArgumentCount [[maybe_unused]] = sizeof...(Args),
            IsPointerToMemberFunction [[maybe_unused]] = true
        };

        template<typename SignalArgs, typename R>
        [[maybe_unused]] static void call(Function f, Obj * const o, void ** const arg) {
            FunctorCall<std::index_sequence_for<Args...>, SignalArgs, R, Function>::call(f, o, arg);
        }
    };

    template<typename Obj, typename Ret, typename... Args>
    struct [[maybe_unused]] FunctionPointer<Ret (Obj::*)(Args...) const> {
        using Object [[maybe_unused]] = Obj ;
        using Arguments [[maybe_unused]]  [[maybe_unused]] = List<Args...>;
        using ReturnType [[maybe_unused]]  [[maybe_unused]] = Ret;
        using Function = Ret (Obj::*)(Args...) const;

        enum {
            ArgumentCount [[maybe_unused]] = sizeof...(Args),
            IsPointerToMemberFunction [[maybe_unused]] = true
        };

        template<typename SignalArgs, typename R>
        [[maybe_unused]] static void call(Function f, Obj * const o, void ** const arg) {
            FunctorCall<std::index_sequence_for<Args...>, SignalArgs, R, Function>::call(f, o, arg);
        }
    };

    template<typename Ret, typename... Args>
    struct [[maybe_unused]] FunctionPointer<Ret (*)(Args...)> {
        using Arguments [[maybe_unused]]  [[maybe_unused]] = List<Args...> ;
        using ReturnType [[maybe_unused]]  [[maybe_unused]] = Ret;
        using Function = Ret (*)(Args...);

        enum {
            ArgumentCount [[maybe_unused]] = sizeof...(Args),
            IsPointerToMemberFunction [[maybe_unused]] = false
        };

        template<typename SignalArgs, typename R>
        [[maybe_unused]] static void call(Function f, void *, void ** const arg) {
            FunctorCall<std::index_sequence_for<Args...>, SignalArgs, R, Function>::call(f, arg);
        }
    };

    template<typename Obj, typename Ret, typename... Args>
    struct [[maybe_unused]] FunctionPointer<Ret (Obj::*)(Args...) noexcept> {
        using Object [[maybe_unused]] = Obj ;
        using Arguments [[maybe_unused]]  [[maybe_unused]] = List<Args...>;
        using ReturnType [[maybe_unused]]  [[maybe_unused]] = Ret;
        using Function = Ret (Obj::*)(Args...) noexcept;

        enum {
            ArgumentCount [[maybe_unused]] = sizeof...(Args),
            IsPointerToMemberFunction [[maybe_unused]] = true
        };

        template<typename SignalArgs, typename R>
        [[maybe_unused]] static void call(Function f, Obj * const o, void ** const arg) {
            FunctorCall<std::index_sequence_for<Args...>, SignalArgs, R, Function>::call(f, o, arg);
        }
    };

    template<typename Obj, typename Ret, typename... Args>
    struct [[maybe_unused]] FunctionPointer<Ret (Obj::*)(Args...) const noexcept> {
        using Object [[maybe_unused]] = Obj ;
        using Arguments [[maybe_unused]]  [[maybe_unused]] = List<Args...> ;
        using ReturnType [[maybe_unused]]  [[maybe_unused]] = Ret ;
        using Function = Ret (Obj::*)(Args...) const noexcept;

        enum {
            ArgumentCount [[maybe_unused]] = sizeof...(Args),
            IsPointerToMemberFunction [[maybe_unused]] = true
        };

        template<typename SignalArgs, typename R>
        [[maybe_unused]] [[maybe_unused]] static void call(Function f, Obj * const o, void ** const arg) {
            FunctorCall<std::index_sequence_for<Args...>, SignalArgs, R, Function>::call(f, o, arg);
        }
    };

    template<typename Ret, typename... Args>
    struct [[maybe_unused]] FunctionPointer<Ret (*)(Args...) noexcept> {
        using Arguments [[maybe_unused]] = List<Args...> ;
        using ReturnType [[maybe_unused]] = Ret ;
        using Function = Ret (*)(Args...) noexcept;
        enum {
            ArgumentCount [[maybe_unused]] = sizeof...(Args),
            IsPointerToMemberFunction [[maybe_unused]] = false
        };

        template<typename SignalArgs, typename R>
        [[maybe_unused]] static void call(Function f, void *, void ** const arg) {
            FunctorCall<std::index_sequence_for<Args...>, SignalArgs, R, Function>::call(f, arg);
        }
    };
    //用于检测两种类型之间是否存在转换的特征，
    //并且该转换不包括窄化转换。
    template <typename T>
    struct NarrowingDetector final { T t[1]{}; }; // from P0608

    template <typename From, typename To, typename Enable = void>
    struct IsConvertibleWithoutNarrowing final : std::false_type {};

    template <typename From, typename To>
    struct IsConvertibleWithoutNarrowing<From, To,
            std::void_t< decltype( NarrowingDetector<To>{ {std::declval<From>()} } ) >
    > final : std::true_type {};

    //检查实际参数。如果它们完全相同，
    //那就不用费心检查是否变窄了；作为副产品，
    //这解决了不完整类型的问题（必须支持，
    //否则他们会在上述特征上出错）。
    template <typename From, typename To, typename Enable = void>
    struct [[maybe_unused]] AreArgumentsConvertibleWithoutNarrowingBase final : std::false_type {};

    template <typename From, typename To>
    struct [[maybe_unused]] AreArgumentsConvertibleWithoutNarrowingBase<From, To,
            std::enable_if_t<
                    std::disjunction_v<std::is_same<From, To>, IsConvertibleWithoutNarrowing<From, To>>
            >
    > final : std::true_type {};

    /*
    检查插槽参数是否与信号参数匹配的逻辑。
    使用方式如下：
    static_assert（检查兼容参数<函数指针<信号>：参数，函数指针<插槽>：参数>：：值）
    */
    template<typename A1, typename A2>
    struct [[maybe_unused]] AreArgumentsCompatible final {
    private:
        [[maybe_unused]] static int test(const std::remove_reference_t<A2> &) {
            return {};
        }

        static char test(...) {
            return {};
        }
    public:
        enum { value = sizeof(test(std::declval<std::remove_reference_t<A1>>())) == sizeof(int) };

        using AreArgumentsConvertibleWithoutNarrowing = AreArgumentsConvertibleWithoutNarrowingBase<std::decay_t<A1>, std::decay_t<A2>>;

        static_assert(AreArgumentsConvertibleWithoutNarrowing::value, "Signal and slot arguments are not compatible (narrowing)");
    };

    template<typename A1, typename A2> struct AreArgumentsCompatible<A1, A2&> final {
        enum { value = false };
    };

    template<typename A> struct AreArgumentsCompatible<A&, A&> final {
        enum { value = true };
    };
    // void as a return value
    template<typename A> struct AreArgumentsCompatible<void, A> final {
        enum { value = true };
    };

    template<typename A> struct AreArgumentsCompatible<A, void> final {
        enum { value = true };
    };

    template<> struct AreArgumentsCompatible<void, void> final{
        enum { value = true };
    };

    template <typename,typename> struct CheckCompatibleArguments final {
        enum { value = false };
    };

    template <> struct CheckCompatibleArguments<List<>, List<>> final {
        enum { value = true };
    };

    template <typename List1> struct CheckCompatibleArguments<List1,List<>> final {
        enum { value = true };
    };

    template <typename Arg1, typename Arg2, typename... Tail1, typename... Tail2>
    struct [[maybe_unused]] CheckCompatibleArguments<List<Arg1, Tail1...>, List<Arg2, Tail2...>> final {
    private:
        using List_1 = List<Tail1...>;
        using List_2 [[maybe_unused]] = List<Tail2...>;
    public:
        enum { value = AreArgumentsCompatible<RemoveConstRef_T<Arg1>,RemoveConstRef_T<Arg2>>::value
                       && CheckCompatibleArguments<List_1, List_2>::value };
    };

    /*
    找到一个functor对象可以接受并且仍然兼容的最大参数数量。
    Value是参数的数量，如果没有匹配项，则为-1。
    */
    template <typename,typename> struct ComputeFunctorArgumentCount;

    template <typename , typename , bool >
    struct ComputeFunctorArgumentCountHelper final {
        enum { Value = -1 };
    };

    template <typename Functor, typename First, typename... ArgList>
    struct ComputeFunctorArgumentCountHelper<Functor, List<First, ArgList...>, false> final
            : ComputeFunctorArgumentCount<Functor,List_Left_V<List<First, ArgList...>, sizeof...(ArgList)>> {};

    template <typename Functor, typename... ArgList>
    struct ComputeFunctorArgumentCount<Functor, List<ArgList...>> {
    private:
        /*以下两个函数可无需有函数体,这里只是为了不会有警告*/
        /**
         * 匹配仿函数,Lambda
         * @tparam F
         * @param f
         * @return int
         */
        template <typename F>
        [[maybe_unused]] static auto test([[maybe_unused]]F f) -> decltype(f.operator()((std::declval<ArgList>())...),int()){
            return {};
        }
        /**
         * 非静态成员函数,静态成员函数,全局函数(包含静态和非静态)
         * @param ...
         * @return
         */
        static char test(...) {
            return {};
        }
    public:
        enum {
            Ok = sizeof(test(std::declval<Functor>())) == sizeof(int),
            Value = Ok ? static_cast<int>(sizeof...(ArgList)) :
                static_cast<int>(ComputeFunctorArgumentCountHelper<Functor, List<ArgList...>, Ok>::Value)
        };
    };

    /* get the return type of a functor, given the signal argument list  */
    template <typename,typename> struct FunctorReturnType;

    template <typename Functor, typename... ArgList>
    struct [[maybe_unused]] FunctorReturnType<Functor, List<ArgList...>> final
            : std::invoke_result<Functor, ArgList...>{ };

    template <typename Functor, typename... ArgList>
    using FunctorReturnType_T [[maybe_unused]] = typename FunctorReturnType<Functor,ArgList...>::type;

    template<typename Func, typename... Args>
    struct [[maybe_unused]] FunctorCallable final {
        using ReturnType = std::invoke_result_t<Func, Args...>;
        using Function [[maybe_unused]] = ReturnType(*)(Args...);
        enum {ArgumentCount [[maybe_unused]] = sizeof...(Args)};
        using Arguments [[maybe_unused]] = List<Args...>;

        template <typename SignalArgs, typename R>
        [[maybe_unused]] static void call(Func &f, void *, void ** const arg) {
            FunctorCall<std::index_sequence_for<Args...>, SignalArgs, R, Func>::call(f, arg);
        }
    };

    template <typename Functor, typename... Args>
    struct HasCallOperatorAcceptingArgs final {
    private:
        template <typename,typename = void>
        struct Test : std::false_type {};
        // We explicitly use .operator() to not return true for pointers to free/static function
        template <typename F>
        struct Test<F,std::void_t<decltype(std::declval<F>().operator()(std::declval<Args>()...))>>
                : std::true_type {};
    public:
        using Type = Test<Functor>;
        static inline constexpr auto value {Type::value};
    };

    template <typename Functor, typename... Args>
    using HasCallOperatorAcceptingArgs_T = typename HasCallOperatorAcceptingArgs<Functor,Args...>::Type;

    template <typename Functor, typename... Args>
    [[maybe_unused]] inline constexpr auto HasCallOperatorAcceptingArgs_v {
            HasCallOperatorAcceptingArgs < Functor, Args...>::value};

    template <typename Func, typename... Args>
    struct CallableHelper {
    private:
        // Could've been std::conditional_t, but that requires all branches to
        // be valid
        [[maybe_unused]] static auto Resolve(std::true_type) -> FunctorCallable<Func, Args...>{return {};}
        static auto Resolve(std::false_type) -> FunctionPointer<std::decay_t<Func>>{return {};}
    public:
        using Type = decltype(Resolve(HasCallOperatorAcceptingArgs_T<std::decay_t<Func>,Args...>{}));
    };

    template <typename Func, typename... Args>
    using CallableHelper_T = typename CallableHelper<Func,Args...>::Type;

    template<typename Func, typename... Args>
    struct [[maybe_unused]] Callable final : CallableHelper_T<Func, Args...> {};

    template<typename Func, typename... Args>
    struct [[maybe_unused]] Callable<Func, List<Args...>> final : CallableHelper_T<Func, Args...> {};

/*
        Wrapper around ComputeFunctorArgumentCount and CheckCompatibleArgument,
        depending on whether \a Functor is a PMF or not. Returns -1 if \a Func is
        not compatible with the \a ExpectedArguments, otherwise returns >= 0.
    */
#if __cplusplus >= 202002L
#define LIKE_WHERE 1 //此处采用C++20以上写法,有三种写法,效果是一样
#if (0 == LIKE_WHERE)
    template<typename Prototype_>
    concept Prototype_t = !std::is_convertible_v<Prototype_, const char *>;

    template<typename Functor_>
    concept Functor_t = !std::is_convertible_v<Functor_, const char *>;

    template<Prototype_t Prototype ,Functor_t Functor>
    [[maybe_unused]] static constexpr int countMatchingArguments() {
#elif (1 == LIKE_WHERE)
    template<typename Prototype ,typename Functor> requires (
        !std::disjunction_v<std::is_convertible<Prototype, const char *>
        ,std::is_convertible<Functor, const char *>>)
    [[maybe_unused]] static constexpr int countMatchingArguments() {
#else
    template<typename Prototype ,typename Functor>
    static constexpr int countMatchingArguments() requires (
        !std::disjunction_v<std::is_convertible<Prototype, const char *>
        ,std::is_convertible<Functor, const char *>>){
#endif

#else
    template<typename Prototype, typename Functor>
    [[maybe_unused]] static constexpr std::enable_if_t<!std::disjunction_v<std::is_convertible<Prototype, const char *>
                    /*,std::is_same<std::decay_t<Prototype>, QMetaMethod>,*/
                    ,std::is_convertible<Functor, const char *>
                    /*,std::is_same<std::decay_t<Functor>, QMetaMethod>*/
    >,int> countMatchingArguments() {
#endif
        using ExpectedArguments = typename FunctionPointer<Prototype>::Arguments;
        using Actual = std::decay_t<Functor>;

        if constexpr (FunctionPointer<Actual>::IsPointerToMemberFunction
                      || FunctionPointer<Actual>::ArgumentCount >= 0) {
            // PMF or free function
            using ActualArguments = typename FunctionPointer<Actual>::Arguments;
            if constexpr (CheckCompatibleArguments<ExpectedArguments, ActualArguments>::value){
                return FunctionPointer<Actual>::ArgumentCount;
            }
            else{
                return -1;
            }
        } else {
            // lambda or functor
            return ComputeFunctorArgumentCount<Actual, ExpectedArguments>::Value;
        }
    }
    /*此处留空有作用*/

    // Helper to detect the context object type based on the functor type:
    // QObject for free functions and lambdas; the callee for member function
    // pointers. The default declaration doesn't have the ContextType typedef,
    // and so non-functor APIs (like old-style string-based slots) are removed
    // from the overload set.
    template <typename, typename = void> struct ContextTypeForFunctor;
#if __cplusplus >= 202002L
    template <typename Func> requires ( //这里括号可以去掉
        !std::disjunction_v<std::is_convertible<Func,const char *>,
        std::is_member_function_pointer<Func>> )
    struct [[maybe_unused]] ContextTypeForFunctor<Func> final {
        using ContextType [[maybe_unused]] = XObject;
    };
#else
    template <typename Func>
    struct [[maybe_unused]] ContextTypeForFunctor<Func,
            std::enable_if_t<!std::disjunction_v<std::is_convertible<Func, const char *>,
                    std::is_member_function_pointer<Func>
            >
            >
    > final {
        using ContextType [[maybe_unused]] = XObject;
    };
#endif

#if __cplusplus >= 202002L
    template <typename Func> requires ( //这里括号可以去掉
        std::disjunction_v<std::negation<std::is_convertible<Func, const char *>>
        ,std::is_member_function_pointer<Func>
        ,std::is_convertible<typename FunctionPointer<Func>::Object *, XObject *>
        >)
    struct [[maybe_unused]] ContextTypeForFunctor<Func> final {
        using ContextType [[maybe_unused]] = typename FunctionPointer<Func>::Object;
    };
#else
    template <typename Func>
    struct [[maybe_unused]] ContextTypeForFunctor<Func,
            std::enable_if_t<std::conjunction_v<std::negation<std::is_convertible<Func, const char *>>,
                    std::is_member_function_pointer<Func>,
                    std::is_convertible<typename FunctionPointer<Func>::Object *, XObject *>
            >
            >
    > final {
        using ContextType [[maybe_unused]] = typename FunctionPointer<Func>::Object;
    };
#endif
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
