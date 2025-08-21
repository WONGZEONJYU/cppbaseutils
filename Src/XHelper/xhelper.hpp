#ifndef X_HELPER_HPP_
#define X_HELPER_HPP_

#include <XHelper/xversion.hpp>
#include <XHelper/xtypetraits.hpp>
#include <XHelper/xutility.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <memory>
#include <functional>
#include <iostream>
#include <type_traits>
#include <thread>
#include <chrono>
#ifdef HAS_QT
#include <QMetaEnum>
#include <QString>
#include <QObject>
#include <QScopedPointer>
#include <QScopedPointer>
#endif

#define X_DISABLE_COPY(...) \
    __VA_ARGS__ (const __VA_ARGS__ &) = delete; \
    __VA_ARGS__ &operator=(const __VA_ARGS__ &) = delete;

#define X_DISABLE_COPY_MOVE(...) \
    X_DISABLE_COPY(__VA_ARGS__) \
    __VA_ARGS__ (__VA_ARGS__ &&) = delete; \
    __VA_ARGS__ &operator=(__VA_ARGS__ &&) = delete;

#define X_DEFAULT_COPY(...) \
    __VA_ARGS__ (const __VA_ARGS__ &) = default;\
    __VA_ARGS__ &operator=(const __VA_ARGS__ &) = default;

#define X_DEFAULT_MOVE(...)\
    __VA_ARGS__ (__VA_ARGS__ &&) = default; \
    __VA_ARGS__ &operator=(__VA_ARGS__ &&) = default;

#define X_DEFAULT_COPY_MOVE(...) \
    X_DEFAULT_COPY(__VA_ARGS__)\
    X_DEFAULT_MOVE(__VA_ARGS__)


#define X_DECLARE_PRIVATE(Class) \
    inline Class##Private* d_func() noexcept \
    { return reinterpret_cast<Class##Private*>(xGetPtrHelper(m_d_ptr_)); } \
    inline const Class##Private* d_func() const noexcept \
    { return reinterpret_cast<const Class##Private *>(xGetPtrHelper(m_d_ptr_)); } \
    friend class Class##Private;

#define X_DECLARE_PRIVATE_D(D_ptr, Class) \
    inline Class##Private* d_func() noexcept \
    { return reinterpret_cast<Class##Private*>(xGetPtrHelper(D_ptr)); } \
    inline const Class##Private* d_func() const noexcept \
    { return reinterpret_cast<const Class##Private *>(xGetPtrHelper(D_ptr)); } \
    friend class Class##Private;

#define X_DECLARE_PUBLIC(...) \
    inline __VA_ARGS__ * x_func() noexcept { return static_cast<__VA_ARGS__ *>(m_x_ptr_); } \
    inline const __VA_ARGS__ * x_func() const noexcept { return static_cast<const __VA_ARGS__ *>(m_x_ptr_); } \
    friend class __VA_ARGS__;

#define X_D(Class) Class##Private * const d{d_func()}
#define X_X(...) __VA_ARGS__ * const x{x_func()}

#define X_ASSERT(cond) ((cond) ? static_cast<void>(0) : XUtils::x_assert(#cond, __FILE__, __LINE__))
#define X_ASSERT_W(cond, where, what) ((cond) ? static_cast<void>(0) : XUtils::x_assert_what(where, what, __FILE__, __LINE__))

#if defined(_MSC_VER) && defined(_WIN32) && defined(_WIN64)
    #define FUNC_SIGNATURE __FUNCSIG__
#else
    #define FUNC_SIGNATURE __PRETTY_FUNCTION__
#endif

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template<typename > class XSingleton;
template<typename > class XHelperClass;

class X_API XUtilsLibErrorLog final {
    XUtilsLibErrorLog() = default;
    static void log(std::string_view const & );
    template<typename > friend class XSingleton;
    template<typename >friend class XHelperClass;
};

template <typename T> inline T *xGetPtrHelper(T *ptr) noexcept { return ptr; }
template <typename Ptr> inline auto xGetPtrHelper(Ptr &ptr) noexcept -> decltype(ptr.get())
{ static_assert(noexcept(ptr.get()), "Smart d pointers for X_DECLARE_PRIVATE must have noexcept get()"); return ptr.get(); }
template <typename Ptr> inline auto xGetPtrHelper(Ptr const &ptr) noexcept -> decltype(ptr.get())
{ static_assert(noexcept(ptr.get()), "Smart d pointers for X_DECLARE_PRIVATE must have noexcept get()"); return ptr.get(); }

template<typename F>
class [[maybe_unused]] Destroyer final {
    X_DISABLE_COPY_MOVE(Destroyer)
    mutable F m_fn_{};
    mutable uint32_t m_is_destroy:1;
public:
#if __cplusplus >= 202002L
    constexpr
#endif
    inline explicit Destroyer(F &&f):
    m_fn_(std::move(f)),m_is_destroy{}{}
#if __cplusplus >= 202002L
    constexpr
#endif
    inline void destroy() const {
        if (!m_is_destroy) {
            m_is_destroy = true;
            m_fn_();
        }
    }
#if __cplusplus >= 202002L
    constexpr
#endif
    inline ~Destroyer() { destroy();}
};

template<typename F2>
class [[maybe_unused]] X_RAII final {
    X_DISABLE_COPY_MOVE(X_RAII)
    mutable F2 m_f2_{};
    mutable uint32_t m_is_destroy_:1;
public:
    template<typename F>
#if __cplusplus >= 202002L
     constexpr
#endif
    inline explicit X_RAII(F &&f1,F2 &&f2):
    m_f2_(std::move(f2)),m_is_destroy_(){f1();}

#if __cplusplus >= 202002L
    constexpr
#endif
    inline void destroy() const {
        if (!m_is_destroy_){
            m_is_destroy_ = true;
            m_f2_();
        }
    }

#if __cplusplus >= 202002L
    constexpr
#endif
    inline ~X_RAII() { destroy();}
};

/**
 * 错误输出,并终止程序
 * @param expr
 * @param file
 * @param line
 */
X_API void x_assert(const char *expr, const char *file,const int &line) noexcept;
X_API void x_assert(const std::string &expr, const std::string &file,const int &line) noexcept;
X_API void x_assert(const std::string_view &expr, const std::string_view &file,const int &line) noexcept;

/**
 * 错误输出,并终止程序
 * @param where
 * @param what
 * @param file
 * @param line
 */
X_API void x_assert_what(const char *where, const char *what,
                   const char *file,const int &line) noexcept;
X_API void x_assert_what(const std::string &where, const std::string &what,
    const std::string &file,const int &line) noexcept;
X_API void x_assert_what(const std::string_view &, const std::string_view &what,
    const std::string_view &file,const int &line) noexcept;

/**
 * 遍历tuple所有元素
 * Pred参数为一个函数,
 * Pred参数有(parm1:std::size & index,
 *  parm2:遍历的元素类型,一般用auto)
 * @tparam Tuple
 * @tparam Pred
 * @param tuple_
 * @param pred_
 */
template<typename Tuple, typename Pred>
[[maybe_unused]] inline void for_each_tuple(Tuple && tuple_, Pred && pred_) {
    (void)std::apply([&pred_]<typename... Args>(Args &&... args) {
        std::size_t index{};
        (std::forward<Pred>(pred_)(std::ref(index),std::forward<Args>(args)), ...);
    },std::forward<Tuple>(tuple_));
}

namespace TuplePrivate {
    template<const std::size_t N,typename... Args>
    inline auto Left_Tuple_n_(const std::tuple<Args...> & tuple_) {
        static_assert(N <= sizeof...(Args), "N must be <= tuple size");
        static_assert(N <= std::tuple_size_v<std::decay_t<decltype(tuple_)>>, "N must be <= tuple size");
        return [&]<const std::size_t... I>(std::index_sequence<I...>)
            ->decltype(std::make_tuple(std::get<I>(tuple_)...)) {
            return std::make_tuple(std::get<I>(tuple_)...);
        }(std::make_index_sequence<N>{});
    }

    // 方法1: 获取后N个元素的通用模板
    template<const std::size_t N, typename... Args>
    inline auto Last_Tuple_n_(const std::tuple<Args...> & tuple_) {
        static_assert(N <= sizeof...(Args), "N must be <= tuple size");
        static_assert(N <= std::tuple_size_v<std::decay_t<decltype(tuple_)>>,"N must be <= tuple size");
        constexpr auto S{sizeof...(Args)  - N};
        return [&]<const std::size_t... I>(std::index_sequence<I...>)
            ->decltype(std::make_tuple(std::get<S + I>(tuple_)...)) {
            return std::make_tuple(std::get<S + I>(tuple_)...);
        }(std::make_index_sequence<N>{});
    }

    // 方法3: 跳过前面元素,取剩余的
    template<std::size_t Skip, typename... Args>
    inline auto skip_front_n_(const std::tuple<Args...>& tuple_) {
        static_assert(Skip <= sizeof...(Args), "Skip count exceeds tuple size");
        constexpr auto Offset{Skip},ReCount{sizeof...(Args) - Skip};
        static_assert(Offset + ReCount <= sizeof...(Args), "Range out of bounds");
        return [&]<const std::size_t ...I>(std::index_sequence<I...>)
        -> decltype(std::make_tuple(std::get<Offset + I>(tuple_)...)){
            return std::make_tuple(std::get<Offset + I>(tuple_)...);
        }(std::make_index_sequence<ReCount>{});
    }
}

/**
 * 获取tuple前N个元素
 * @tparam N
 * @tparam Tuple
 * @param tuple_
 * @return
 */
template<const std::size_t N,typename Tuple>
[[maybe_unused]] inline auto Left_Tuple(const Tuple & tuple_) {
    return TuplePrivate::Left_Tuple_n_<N>(tuple_);
}

/**
 * 获取tuple后N个元素
 * @tparam N
 * @tparam Tuple
 * @param tuple_
 * @return decltype(TuplePrivate::Last_Tuple_n_<N,Tuple>(tuple_))
 */
template<const std::size_t N,typename Tuple>
[[maybe_unused]] inline auto Last_Tuple(const Tuple & tuple_) {
    return TuplePrivate::Last_Tuple_n_<N>(tuple_);
}

/**
 * 获取tuple跳过N个元素后面的所有元素
 * @tparam N
 * @tparam Tuple
 * @param tuple_
 * @return decltype(TuplePrivate::skip_front_n_<Skip>(tuple_))
 */
template<const std::size_t N,typename Tuple>
[[maybe_unused]] inline auto SkipFront_Tuple(const Tuple &tuple_) {
    return TuplePrivate::skip_front_n_<N>(tuple_);
}

[[maybe_unused]] X_API std::string toLower(std::string &);

[[maybe_unused]] X_API std::string toLower(std::string &&);

[[maybe_unused]] X_API std::string toLower(const std::string &);

[[maybe_unused]] X_API std::string toLower(std::string_view &);

[[maybe_unused]] X_API std::string toLower(std::string_view &&);

[[maybe_unused]] X_API std::string toLower(const std::string_view &);

[[maybe_unused]] X_API std::string toUpper(std::string &);

[[maybe_unused]] X_API std::string toUpper(std::string &&);

[[maybe_unused]] X_API std::string toUpper(const std::string &);

[[maybe_unused]] X_API std::string toUpper(std::string_view &);

[[maybe_unused]] X_API std::string toUpper(std::string_view &&);

[[maybe_unused]] X_API std::string toUpper(const std::string_view &);

enum class ConnectionType {
    AutoConnection,
    UniqueConnection
};

namespace XPrivate {

    template<typename Object>
    struct Non_INSTANCE_MEM {
    private:
        template<typename ,typename = void>
        struct Test : std::true_type {};

        template<typename O>
        struct Test<O, std::void_t< decltype(O::s_instance_) > > : std::false_type {};
    public:
        enum {value = Test<Object>::value};
    };

    template<typename Object>
    inline constexpr bool Non_INSTANCE_MEM_v {Non_INSTANCE_MEM<Object>::value};

    template<typename Object>
    struct Has_S_INSTANCE_MEM {
    private:
        template<typename ,typename = void>
        struct Test : std::false_type {};

        template<typename O>
        struct Test<O, std::void_t< decltype(O::s_instance_) > > : std::true_type {
            static_assert(std::is_same_v<Object *,decltype(O::s_instance_)>
                ,"The type must be Object pointer type");
        };
    public:
        enum {value = Test<Object>::value};
    };

    template<typename Object>
    inline constexpr bool Has_S_INSTANCE_MEM_v { Has_S_INSTANCE_MEM< Object >::value };

    template<typename Object>
    struct Has_X_HELPER_CLASS_Macro {
    private:
        static_assert(std::is_object_v<Object>,"typename Object don't Object type");

        template<typename T>
        inline static char test( void (T::*)() ){throw;}

        inline static int test( void (Object::*)() ){throw;}
    public:
        enum { value = sizeof(test(&Object::checkFriendXHelperClass_)) == sizeof(int) };
    };

    template<typename Object>
    inline constexpr bool Has_X_HELPER_CLASS_Macro_v { Has_X_HELPER_CLASS_Macro<Object>::value };

    template<typename Object,typename ...Args>
    struct Has_construct_Func {
    private:
        static_assert(std::is_object_v<Object>,"typename Object don't Object type");
    #if __cplusplus >= 202002L
        template<typename O,typename ...A>
        inline static auto test(int) -> std::true_type
            requires ( ( sizeof( std::declval<O>().construct_( ( std::declval< std::decay_t< A > >() )... ) )
                > static_cast<std::size_t>(0) ) )
        {throw ;}
    #else
        #if 0 //只能二选一
            template<typename O,typename ...A>
            inline static auto test(int)
                -> std::enable_if_t< ( sizeof(std::declval<O>().construct_( (std::declval< std::decay_t< A > >())...) )
                    > static_cast<std::size_t>(0) )
                    ,std::true_type >
            {throw ;}
        #else
            template<typename O,typename ...A>
            inline static auto test(int)
            -> decltype( sizeof( std::declval<O>().construct_( (std::declval< std::decay_t< A > >())...) )
                > static_cast<std::size_t>(0)
                    , std::true_type{} )
            {throw;}
        #endif
    #endif
        template<typename ...>
        inline static auto test(...) -> std::false_type {throw ;}
    public:
        enum { value = decltype(test<Object,Args...>(0))::value };
    };

    template<typename ...Args>
    inline constexpr bool Has_construct_Func_v {Has_construct_Func<Args...>::value};

    template<typename Object,typename ...Args>
    struct is_private_mem_func {
        static_assert(std::is_object_v<Object>,"typename Object don't Object type");
    private:
    #if __cplusplus >= 202002L
        template<typename O,typename ...A>
        inline static auto test(int) -> std::false_type
            requires (
                ( sizeof( std::declval<O>().construct_( std::declval< std::decay_t< A > >()...) ) > static_cast<std::size_t>(0) )
                    || std::is_same_v< decltype( std::declval<O>().construct_( std::declval< std::decay_t< A > >()...) ),void >
            )
        {throw ;}
    #else
        #if 0 //只能二选一
            template<typename O,typename ...A>
            inline static auto test(int)
                -> std::enable_if_t< std::is_same_v< decltype( std::declval<O>().construct_( (std::declval< std::decay_t< A > >())...) ) ,void >
                     || ( sizeof( std::declval<O>().construct_( (std::declval< std::decay_t< A > >())...) ) > static_cast<std::size_t>(0) )
                        ,std::false_type >
                {throw ;}
        #else
            template<typename O,typename ...A>
            inline static auto test(int)
                -> decltype( std::is_same_v< decltype(std::declval<O>().construct_((std::declval< std::decay_t< A > >())...)), void >
                    || ( sizeof( std::declval<O>().construct_( (std::declval< std::decay_t< A > >())... ) ) > static_cast<std::size_t>(0) )
                        ,std::false_type {} )
                { throw ; }
        #endif
    #endif

        template<typename ...>
        inline static auto test(...) ->std::true_type {throw ;}
    public:
        enum { value = decltype(test<Object,Args...>(0))::value };
    };

    template<typename ...Args>
    inline constexpr bool is_private_mem_func_v{ is_private_mem_func<Args...>::value };

    template<typename T, typename... Args>
    struct is_default_constructor_accessible {
    private:
        enum {
            result = std::disjunction_v< std::is_constructible< T, std::decay_t< Args >... >
                    ,std::is_nothrow_constructible< T ,std::decay_t<Args>... >
                    ,std::is_trivially_constructible< T ,std::decay_t<Args>... >
            >
        };

        template<typename > struct is_copy_move_constructor {
            enum { value = false };
        };

    #if __cplusplus >= 202002L
        template<typename ...AS> requires(sizeof...(AS) == 1)
        struct is_copy_move_constructor<std::tuple<AS...>> {
            using Tuple_ = std::tuple<AS...>;
            using First_ = std::tuple_element_t<0, Tuple_>;
            enum {
                value = std::disjunction_v<
                    std::is_same<First_, T &>,
                    std::is_same<First_, const T &>,
                    std::is_same<First_, T &&>,
                    std::is_same<First_, const T &&>
                >
            };
        };
    #else
        template<> struct is_copy_move_constructor<std::tuple<>> {
            enum { value = false };
        };
        template<typename ...AS>
        struct is_copy_move_constructor<std::tuple<AS...>> {
        private:
            using Tuple_ = std::tuple<AS...>;
            using First_ = std::tuple_element_t<0, Tuple_>;
        public:
            enum {
                value = std::disjunction_v<
                    std::is_same<First_, T &>,
                    std::is_same<First_, const T &>,
                    std::is_same<First_, T &&>,
                    std::is_same<First_, const T &&>>
            };
        };
    #endif

    public:
        enum {value = result && !is_copy_move_constructor<std::tuple<Args...>>::value};
    };

    template<typename ...Args>
    inline constexpr bool is_default_constructor_accessible_v { is_default_constructor_accessible<Args...>::value };

    template<typename Object>
    struct is_destructor_private {
    private:
        static_assert(std::is_object_v<Object>,"typename Object don't Object type");

        template<typename O>
        inline static auto test(int)
        -> decltype(std::declval<O>().~O(),std::false_type{}){
            throw ;
        }

        template<typename >
        inline static auto test(...) -> std::true_type {
            throw ;
        }

    public:
        enum {value = decltype(test<Object>(0))::value };
    };

    template<typename Object>
    inline constexpr bool is_destructor_private_v { is_destructor_private<Object>::value };

    template<typename Tp_>
    struct Allocator_ {

        using value_type = Tp_;

        constexpr Allocator_() = default;

        template<class U>
        [[maybe_unused]] constexpr explicit Allocator_(const Allocator_ <U>&) noexcept {}

        [[maybe_unused]] [[nodiscard]] static auto allocate(std::size_t const n) noexcept -> value_type * {
            while (true) {
                if (auto const ptr{ std::malloc(sizeof(value_type) * n) }) {
                    return static_cast<value_type*>(ptr);
                }
            }
        }

        [[maybe_unused]] static void deallocate(value_type * const ptr, std::size_t) noexcept {
            std::free(ptr);
        }
    };

    template<typename T, typename U>
    bool operator==(const Allocator_ <T>&, const Allocator_ <U>&) { return true; }

    template<typename T, typename U>
    bool operator!=(const Allocator_ <T>&, const Allocator_ <U>&) { return false; }

#define STATIC_ASSERT_P \
    static_assert( std::is_object_v< Object >,"typename Object is not an class or struct" ); \
                        \
    static_assert( std::is_final_v< Object > ,"Object must be a final class" ); \
                        \
    static_assert( XPrivate::Has_X_HELPER_CLASS_Macro_v< Object > \
            ,"No X_HELPER_CLASS in the class!" ); \
                        \
    static_assert( XPrivate::Has_construct_Func_v< Object ,std::decay_t<Args2>... > \
            ,"bool Object::construct_(...) non static member function absent!" ); \
                        \
    static_assert( XPrivate::is_private_mem_func_v< Object ,std::decay_t<Args2>... > \
            ,"bool Object::construct_(...) must be a private non static member function!" ); \
                        \
    static_assert( !XPrivate::is_default_constructor_accessible_v< Object ,std::decay_t< Args1 >... > \
            ,"The Object (...) constructor (non copy and non move) must be a private member function!" );

} //namespace XPrivate;

template<typename ...Args>
using Parameter = std::tuple<Args...>;

/**
 * example:
 * class Test : public xtd::XHelperClass<Test> {
 *      X_HELPER_CLASS
 *      bool construct_(...) { initCode...; return true;} //此为二阶构造,也必须是私有
 *      Test(...) {xxxCode...;} //有参数非拷贝非移动构造函数必须是私有的
 *      Test() = default; //如果不需无参构造函数做任何事,需显式提供私有默认构造函数
 *      Test() {xxxCode...;} //需无参构造函数做事
 *  public:
 *      void xxx(...) {xxxCode...;}
 *  private:
 *      void xxxx(...) {xxxCode...;}
 * };
 *
 * int main(...) {
 *   auto p = Test::Create();
 *   delete p;
 *   auto sptr = Test::CreateSharedPtr();
 *   auto uptr = Test::CreateUniquePtr();
 *   //如果构造函数有参数和二阶构造有参数:
 *   auto p1 = Test::Create(xtd::Parameter{1,...},xtd::Parameter{10,...});
 *   delete p1;
 *   智能指针版本同理
 *
 *   delete Test::Create({},xtd::Parameter{10,...});
 *   delete Test::Create(xtd::Parameter{10,...},{});
 * }
 *
 * @tparam Tp_
 */

template<typename Tp_>
class XHelperClass {

    using Object_t = std::decay_t< RemoveRef_T<Tp_> >;

    static_assert(std::negation_v< std::is_pointer< Object_t > >,"Tp_ Cannot be pointer type");

#if __cplusplus < 202002L
    template< typename Tuple1_ ,typename Tuple2_ ,std::size_t...I1 ,std::size_t...I2 >
    inline static constexpr auto CreateHelper(Tuple1_ && args1,Tuple2_ && args2
            ,std::index_sequence< I1... >,std::index_sequence< I2... >) noexcept -> Object_t *
    {
        try{
            ObjectUPtr obj { new Object_t( std::get< I1 >( std::forward< Tuple1_ >( args1 ) )... )
                    ,Deleter {} };
            return obj->construct_( std::get< I2 >( std::forward< Tuple2_ >( args2 ) )... ) ? obj.release() : nullptr;
        } catch (const std::exception &) {
            return nullptr;
        }
    }
#endif

    template<typename Tuple_>
    inline static constexpr auto indices(Tuple_ &&) noexcept
    -> std::make_index_sequence< std::tuple_size_v< std::decay_t< Tuple_ > > >
    {return {};}

protected:
    template<typename Object> struct Destructor_ {

        constexpr Destructor_() = default;

        template<typename U>
        [[maybe_unused]] constexpr explicit Destructor_(const Destructor_<U> &) {}

        inline static void cleanup(const Object * const pointer) noexcept {
            using IsIncompleteType = char[ sizeof(Object) ? 1 : -1 ];
            (void) sizeof(IsIncompleteType);
            delete pointer;
        }

        void operator()(const Object * const pointer) const noexcept
        {cleanup(pointer);}
    };

    using Deleter = Destructor_< Object_t >;

public:
    using Object = Object_t;
    using ObjectSPtr = std::shared_ptr< Object >;
    using ObjectUPtr = std::unique_ptr< Object , Deleter >;

    template<typename ...Args1,typename ...Args2>
    [[nodiscard]] inline constexpr static Object * Create( Parameter< Args1... > && args1 = {},
                                  Parameter< Args2...> && args2 = {} ) noexcept
    {
        static_assert( std::disjunction_v< std::is_base_of< XHelperClass ,Object >
            ,std::is_convertible<Object,XHelperClass >
        > ,"Object must inherit from Class XHelperClass" );

        STATIC_ASSERT_P

#if __cplusplus >= 202002L
        return [&]< std::size_t ...I1 ,std::size_t...I2 >( std::index_sequence< I1... > ,std::index_sequence< I2... > )
            noexcept -> Object *
        {
            try{
                ObjectUPtr obj { new Object( std::get<I1>( std::forward< decltype( args1 ) >( args1 ) )... )
                    ,Deleter {} };
                return obj->construct_( std::get<I2>( std::forward< decltype( args2 ) >( args2 ) )... ) ? obj.release() : nullptr;
            } catch (const std::exception &) {
                return nullptr;
            }
        }( indices( args1 ) ,indices( args2 ) );
#else
        return CreateHelper(args1,args2,indices(args1),indices(args2));
#endif
    }

    template<typename ...Args1,typename ...Args2>
    [[nodiscard]] inline static constexpr auto CreateSharedPtr ( Parameter< Args1...> && args1 = {}
        ,Parameter< Args2...> && args2 = {} ) noexcept -> ObjectSPtr {
        return { Create( std::forward< decltype( args1 ) >( args1 )
            ,std::forward< decltype( args2 ) >( args2 ) ) ,Deleter{}
            ,XPrivate::Allocator_< Object >{} };
    }

    template<typename ...Args1,typename ...Args2>
    [[nodiscard]] inline static constexpr auto CreateUniquePtr ( Parameter< Args1... > && args1 = {}
        ,Parameter< Args2... > && args2 = {} ) noexcept -> ObjectUPtr {
        return { Create( std::forward< decltype( args1 ) >( args1 )
            ,std::forward< decltype( args2 ) >( args2 ) ) ,Deleter{} };
    }

#ifdef HAS_QT

    using ObjectQUPtr = QScopedPointer<Object,Deleter>;
    using ObjectQSPtr = QSharedPointer<Object>;

    template<typename ...Args1,typename ...Args2>
    [[nodiscard]] [[maybe_unused]] inline static constexpr auto CreateQScopedPointer ( Parameter< Args1... > && args1 = {}
            ,Parameter< Args2... > && args2 = {} ) noexcept -> ObjectQUPtr {
        return ObjectQUPtr{ Create( std::forward< decltype( args1 ) >( args1 )
                ,std::forward< decltype( args2 ) >( args2 ) ) };
    }

    template<typename ...Args1,typename ...Args2>
    [[nodiscard]] inline static constexpr auto CreateQSharedPointer ( Parameter< Args1...> && args1 = {}
            ,Parameter< Args2...> && args2 = {} ) noexcept -> ObjectQSPtr {
        try{
            return ObjectQSPtr{ Create( std::forward< decltype( args1 ) >( args1 )
                    ,std::forward< decltype( args2 ) >( args2 ) ) ,Deleter{} };
        }catch (std::exception const &) {
            return ObjectQSPtr{};
        }
    }

   #if __cplusplus >= 202002L
   #define LIKE_WHICH 1
   #if LIKE_WHICH == 1
       template<typename ENUM_> requires (static_cast<bool>(QtPrivate::IsQEnumHelper<ENUM_>::Value))
       inline static QString getEnumTypeAndValueName(ENUM_ && enumValue) {
   #elif LIKE_WHICH == 2
       template<typename ENUM_>
       inline static QString getEnumTypeAndValueName(ENUM_ && enumValue)
       requires (static_cast<bool>(QtPrivate::IsQEnumHelper<ENUM_>::Value)) {
   #else
       template<typename T>
       concept ENUM_T = static_cast<bool>(QtPrivate::IsQEnumHelper<T>::Value);
       template<ENUM_T ENUM_>
       inline static QString getEnumTypeAndValueName(ENUM_ && enumValue) {
   #endif
   #else
       template<typename ENUM_>
       inline static std::enable_if_t<QtPrivate::IsQEnumHelper<ENUM_>::Value, QString>
       getEnumTypeAndValueName(ENUM_ && enumValue) {
   #endif
   #undef LIKE_WHICH
           if constexpr ( std::is_object_v<Object_t> ) {
               static_assert(XPrivate::Has_X_HELPER_CLASS_Macro_v<Object_t>
                       ,"No X_HELPER_CLASS in the class!");
           }
           const auto metaObj {qt_getEnumMetaObject(enumValue)};
           const auto EnumTypename {qt_getEnumName(enumValue)};
           const auto enumIndex {metaObj->indexOfEnumerator(EnumTypename)};
           const auto metaEnum {metaObj->enumerator(enumIndex)};
           const auto enumValueName{metaEnum.valueToKey(enumValue)};
           return QString("%1::%2").arg(EnumTypename).arg(enumValueName);
       }
   #if __cplusplus >= 202002L
   #define LIKE_WHICH 1
       /*std::disjunction_v<std::is_same<QObject,T>,std::is_base_of<QObject,T> ==
        * std::is_same_v<QObject,T> || std::is_base_of_v<QObject,T>
       */
   #if (LIKE_WHICH == 1)
       template<typename T> requires(std::is_same_v<QObject,T> || std::is_base_of_v<QObject,T>)
       inline static T *findChildByName(QObject* parent, const QString& objectname) {
   #elif (LIKE_WHICH == 2 )
       template<typename T>
       inline static T *findChildByName(QObject* parent, const QString& objectname)
       requires(std::is_same_v<QObject,T> || std::is_base_of_v<QObject,T>) {
   #else
       template<typename T>
       concept QObject_t = std::is_same_v<QObject,T> || std::is_base_of_v<QObject,T>;
       template<QObject_t T>
       inline static T *findChildByName(QObject* parent, const QString& objectname) {
   #endif
   #else
       template <typename T>
       inline static std::enable_if_t<std::disjunction_v<std::is_same<QObject,T>,std::is_base_of<QObject,T>>,T*>
       findChildByName(QObject* parent, const QString& objectname) {
   #endif
           if constexpr ( std::is_object_v<Object_t> ) {
               static_assert(XPrivate::Has_X_HELPER_CLASS_Macro_v<Object_t>
                       ,"No X_HELPER_CLASS in the class!");
           }
           foreach (QObject* child, parent->children()) {
               if (child->objectName() == objectname) {
                   return qobject_cast<T*>(child);
               }
           }
           return nullptr;
       }
       template<typename ...Args>
       inline static QMetaObject::Connection ConnectHelper(Args && ...args) {
           if constexpr ( std::is_object_v<Object_t> ) {
               static_assert(XPrivate::Has_X_HELPER_CLASS_Macro_v<Object_t>
                       ,"No X_HELPER_CLASS in the class!");
           }
           return QObject::connect(std::forward<Args>(args)...);
       }
#endif
protected:
    XHelperClass() = default;
    template<typename> friend class XSingleton;
};

template<typename Tp_>
class XSingleton : protected XHelperClass<Tp_> {
    using Base_ = XHelperClass<Tp_>;
    static_assert(std::is_object_v<typename Base_::Object>,"Tp_ must be a class or struct type!");
public:
    using Object = typename Base_::Object;
    using SingletonPtr = typename Base_::ObjectSPtr;

    template<typename ...Args1,typename ...Args2>
    inline static auto UniqueConstruction([[maybe_unused]] Parameter<Args1...> && args1 = {}
        , [[maybe_unused]] Parameter<Args2...> && args2 = {}) noexcept -> SingletonPtr
    {
        static_assert( XPrivate::is_destructor_private_v< Object >
                , "destructor( ~Object() ) must be private!" );

        static_assert( std::disjunction_v< std::is_base_of< XSingleton ,Object >
                ,std::is_convertible<Object,XSingleton >
        > ,"Object must inherit from Class XSingleton" );

        STATIC_ASSERT_P

        Allocator_([&args1,&args2] {
            return Base_::CreateSharedPtr(std::forward< decltype(args1) >(args1)
                    ,std::forward<decltype(args2) >(args2));
        });

        return data();
    }

    [[maybe_unused]] inline static auto instance() noexcept -> SingletonPtr
    {return data();}

    [[maybe_unused]] [[nodiscard]] inline static constexpr bool isConstruct() noexcept
    {return static_cast<bool >(data());}

#ifdef HAS_QT
    using QSingletonPtr = typename Base_::ObjectQSPtr;

    template<typename ...Args1,typename ...Args2>
    [[maybe_unused]] [[nodiscard]]
    inline static auto QUniqueConstruction([[maybe_unused]] Parameter<Args1...> && args1 = {}
            , [[maybe_unused]] Parameter<Args2...> && args2 = {}) noexcept -> QSingletonPtr
    {
        static_assert( XPrivate::is_destructor_private_v< Object >
                , "destructor( ~Object() ) must be private or protected!" );

        static_assert( std::disjunction_v<
                std::conjunction< std::is_base_of< QObject ,Object >
                ,std::is_base_of< XSingleton ,Object > >
                ,std::is_convertible<Object,XSingleton >
        > ,"Object must inherit from Class QObject and Class XSingleton!" );

        STATIC_ASSERT_P

        Allocator_([&args1,&args2]{
            return Base_::CreateQSharedPointer( std::forward< decltype( args1 ) >( args1 )
                    ,std::forward< decltype( args2 ) >( args2 ) );
        });
        return qdata();
    }

    [[maybe_unused]] inline static auto qInstance() noexcept -> QSingletonPtr
    {return qdata();}

    [[maybe_unused]] [[nodiscard]] inline static constexpr bool isQConstruct() noexcept
    {return static_cast<bool >(qdata());}
#endif

private:
    inline static auto initFlag() noexcept -> std::once_flag &
    {static std::once_flag flag{};return flag;}

    inline static auto data() noexcept -> SingletonPtr &
    {static SingletonPtr d{};return d;}

#ifdef HAS_QT
    inline static auto qdata() noexcept -> QSingletonPtr &
    {static QSingletonPtr d{};return d;}
#endif

    template<typename Callable>
    inline static void Allocator_([[maybe_unused]] Callable && callable) noexcept {

        static_assert(XPrivate::Has_S_INSTANCE_MEM_v<Object>,
            "Derived classes must have a static member variable named s_instance_, "
            "the type must be the class type of the derived class itself, and it must be private");

        static_assert(XPrivate::Non_INSTANCE_MEM_v<Object>
            ,"The s_instance static member variable must be private");

        std::call_once(initFlag(),[&]{

            std::ostringstream err_msg{};

            for (int i{}; i < 5; ++i) {

                if (auto ptr { std::forward<Callable>(callable)() }) {
#ifdef HAS_QT
                    using ptrType = std::decay_t < std::invoke_result_t< Callable > >;

                    if constexpr (std::is_same_v < SingletonPtr, ptrType > ) {
                        data().swap(ptr);
                    } else if constexpr ( std::is_same_v< QSingletonPtr , ptrType > ) {
                        qdata().swap(ptr);
                    }else {
                        static_assert(std::disjunction_v< std::is_same < SingletonPtr, ptrType >
                                      , std::is_same < QSingletonPtr, ptrType > >
                                , "no SingletonPtr and  QSingletonPtr!");
                    }
#else
                    data() = std::move(ptr);
#endif
                    return;
                }

                err_msg << FUNC_SIGNATURE << typeName<Object>()
                        << " memory allocation failed, retry in 1 second\n";

                XUtilsLibErrorLog::log(err_msg.str());
                std::cerr << err_msg.str();
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }

            err_msg.clear();
            err_msg  << FUNC_SIGNATURE << " : " << typeName<Object>()
                     << " memory allocation failed"
                        ",The maximum number of retries has been reached!\n";

            XUtilsLibErrorLog::log(err_msg.str());
            std::cerr << err_msg.str();
        });
    }

protected:
    XSingleton() = default;
    X_DISABLE_COPY_MOVE(XSingleton)
};

#undef STATIC_ASSERT_P

template<typename T,typename ...Args>
[[maybe_unused]] inline auto makeUnique(Args && ...args) noexcept -> std::unique_ptr<T> {
    try{
        return std::unique_ptr<T>{ new T( std::forward<Args>(args)... ) };
    }catch (const std::exception &) {
        return {};
    }
}

template<typename T,typename ...Args>
[[maybe_unused]] inline auto makeShared(Args && ...args) noexcept -> std::shared_ptr<T> {
    try{
        return std::make_shared<T>(std::forward<Args>(args)...);
    }catch (const std::exception &){
        return {};
    }
}

using HelperClass [[maybe_unused]] = XHelperClass<void>;

#define X_HELPER_CLASS \
private: \
    inline void checkFriendXHelperClass_(){ X_ASSERT_W( false ,FUNC_SIGNATURE \
        ,"This function is used for checking, please don't call it!"); \
    } \
    template<typename> friend class XUtils::XHelperClass; \
    template<typename> friend struct XUtils::XPrivate::Has_X_HELPER_CLASS_Macro; \
    template<typename ,typename ...> friend struct XUtils::XPrivate::Has_construct_Func; \
    template<typename > friend struct XUtils::XPrivate::Has_S_INSTANCE_MEM;\
    template<typename> friend class XUtils::XSingleton;

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
