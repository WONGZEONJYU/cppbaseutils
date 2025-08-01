#ifndef X_HELPER_HPP_
#define X_HELPER_HPP_

#include <XHelper/xversion.hpp>
#include <XHelper/xoverload.hpp>
#include <XHelper/xtypetraits.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <memory>
#include <functional>
#include <type_traits>
#ifdef HAS_QT
#include <QMetaEnum>
#include <QString>
#include <QObject>
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

template <typename T> inline T *xGetPtrHelper(T *ptr) noexcept { return ptr; }
template <typename Ptr> inline auto xGetPtrHelper(Ptr &ptr) noexcept -> decltype(ptr.get())
{ static_assert(noexcept(ptr.get()), "Smart d pointers for X_DECLARE_PRIVATE must have noexcept get()"); return ptr.get(); }
template <typename Ptr> inline auto xGetPtrHelper(Ptr const &ptr) noexcept -> decltype(ptr.get())
{ static_assert(noexcept(ptr.get()), "Smart d pointers for X_DECLARE_PRIVATE must have noexcept get()"); return ptr.get(); }

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

#define X_D(Class) Class##Private * const d{d_func()};
#define X_X(...) __VA_ARGS__ * const x{x_func()};

#define X_ASSERT(cond) ((cond) ? static_cast<void>(0) : xtd::x_assert(#cond, __FILE__, __LINE__))
#define X_ASSERT_W(cond, where, what) ((cond) ? static_cast<void>(0) : xtd::x_assert_what(where, what, __FILE__, __LINE__))

#define X_IN
#define X_OUT
#define X_IN_OUT

#if defined(_MSC_VER) && defined(_WIN32) && defined(_WIN64)
    #define FUNC_SIGNATURE __FUNCSIG__
#else
    #define FUNC_SIGNATURE __PRETTY_FUNCTION__
#endif

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template<typename F>
class [[maybe_unused]]  Destroyer final {
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
 * 创建std::unique_ptr<T>,不抛异常
 * @tparam T
 * @tparam Args
 * @param args
 * @return std::unique_ptr<T>
 */
template<typename T, typename ... Args>
[[maybe_unused]] [[nodiscard]] inline std::unique_ptr<T> make_Unique(Args && ...args) noexcept {
    try{
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }catch (const std::exception &){
        return {};
    }
}

/**
 * 创建std::shared_ptr<T>,不抛异常
 * @tparam T
 * @tparam Args
 * @param args
 * @return std::shared_ptr<T>
 */
template<typename T, typename ... Args>
[[maybe_unused]] [[nodiscard]] inline std::shared_ptr<T> make_Shared(Args && ...args) noexcept {
    try{
        return std::make_shared<T>(std::forward<Args>(args)...);
    }catch (const std::exception &){
        return {};
    }
}

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
    template<typename Obj>
    struct Has_X_HELPER_CLASS_Macro {
    private:
        static_assert(std::is_object_v<Obj>,"typename Obj don't Object type");
        template<typename T>
        inline static char test( void (T::*)() ){throw;}
        inline static int test( void (Obj::*)() ){throw;}
    public:
        enum {value = sizeof(test(&Obj::checkFriendXHelperClass_)) == sizeof(int)};
    };

    template<typename Obj>
    inline constexpr bool Has_X_HELPER_CLASS_Macro_v{Has_X_HELPER_CLASS_Macro<Obj>::value};

    template<typename Obj,typename ...Args>
    struct Has_construct_Func {
    private:
        static_assert(std::is_object_v<Obj>,"typename Obj don't Object type");
        template<typename Object,typename ...AS>
        inline static char test( bool (Object::*)(AS...) ){throw;}
        inline static int test( bool (Obj::*)(Args...) ){throw;}
    public:
        enum { value = sizeof( test( xOverload<Args...>( &Obj::construct_ ) ) ) == sizeof(int) };
    };

    template<typename ...Args>
    inline constexpr bool Has_construct_Func_v {Has_construct_Func<Args...>::value};

    template<typename Obj,typename ...Args>
    struct is_private_mem_func {
    private:
    #if __cplusplus >= 202002L
        template<typename Object>
        static std::false_type test(void *) requires (
                std::is_same_v<decltype(std::declval<Object>().construct_( (std::declval<Args>())...) ),void>
                || (sizeof(std::declval<Object>().construct_( (std::declval<Args>())...) ) > static_cast<std::size_t>(0))
            )
        {return {};}
    #else
        template<typename Object>
        static std::enable_if_t<
            std::is_same_v<decltype(std::declval<Object>().construct_( (std::declval<Args>())...) ),void>
        ,std::false_type> test(void *)
        {return {};}

        template<typename Object>
        static std::enable_if_t<
            (sizeof(std::declval<Object>().construct_( (std::declval<Args>())...) ) > static_cast<std::size_t>(0))
        ,std::false_type> test(std::nullptr_t)
        {return {};}
    #endif
        template<typename >
        static std::true_type test(...) {return {};}
    public:
        static constexpr bool value {decltype(test<Obj>(nullptr))::value};
    };

    template<typename ...Args>
    inline constexpr auto is_private_mem_func_v{ is_private_mem_func<Args...>::value };

    template<typename T, typename... Args>
    struct is_default_constructor_accessible {
    private:
        enum {
            result = std::disjunction_v< std::is_constructible<T, Args...>
                    ,std::is_nothrow_constructible<T, Args...>
                    ,std::is_trivially_constructible<T,Args...>
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
    inline constexpr bool is_default_constructor_accessible_v {is_default_constructor_accessible<Args...>::value};
}

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

    using Object_t = RemoveRef_T<Tp_>;
    using Type_t = RemoveRef_T<Tp_>;

    static_assert(std::negation_v< std::is_pointer< Type_t > >,"Tp_ Cannot be pointer type");

    template<typename Tuple_>
    inline static constexpr auto indices(Tuple_ &&) noexcept
        -> std::make_index_sequence< std::tuple_size_v< std::decay_t< Tuple_ > > >
    {
        return std::make_index_sequence< std::tuple_size_v< std::decay_t< Tuple_ > > >{};
    }

    template<typename Tuple1_ ,typename Tuple2_ ,std::size_t...I1 ,std::size_t...I2>
    inline static constexpr Object_t * CreateHelper(Tuple1_ && args1,Tuple2_ && args2,
        std::index_sequence<I1...>,std::index_sequence<I2...>) noexcept
    {
        auto obj { make_Unique<Object>( std::get<I1>( std::forward< decltype(args1) >( args1 ) )... ) };
        return obj && obj->construct_( std::get<I2>( std::forward< decltype(args2) >( args2) )... ) ? obj.release() : nullptr;
    }
public:
    using Object = Object_t;
    using Type = Type_t;
    template<typename ...Args1,typename ...Args2>
    [[nodiscard]] inline constexpr static Object * Create( Parameter<Args1...> const & args1 = {},
                                  Parameter<Args2...> const & args2 = {} ) noexcept
    {
        static_assert(std::is_object_v<Object>,"typename Object is not an object");

        static_assert(std::conjunction_v< std::is_base_of<XHelperClass,Object>
                        ,std::is_convertible<Object,XHelperClass>
                        > ,"Object must inherit from Class HelperClass");

        static_assert(XPrivate::Has_X_HELPER_CLASS_Macro_v<Object>
                      ,"No X_HELPER_CLASS in the class!");

        static_assert(XPrivate::Has_construct_Func_v<Object,Args2...>
                    ,"bool Object::construct_(...) non static member function absent!");

        static_assert(XPrivate::is_private_mem_func_v<Object,Args2...>
                    ,"bool Object::construct_(...) must be a private non static member function!");

        static_assert(!XPrivate::is_default_constructor_accessible_v<Object,Args1...>
                    ,"The Object (...) constructor (non copy and non move) must be a private member function!");
#if __cplusplus >= 201402L
        return [&]<std::size_t ...I1,std::size_t...I2>(std::index_sequence<I1...>,std::index_sequence<I2...>)-> Object * {
            auto obj { make_Unique<Object>( std::get<I1>( std::forward< decltype(args1) >( args1 ) )... ) };
            return obj && obj->construct_( std::get<I2>( std::forward< decltype(args2) >( args2 ) )... ) ? obj.release() : nullptr;
        }(indices(args1),indices(args2));
#else
        return CreateHelper(args1,args2,indices(args1),indices(args2));
#endif
    }

    template<typename ...Args1,typename ...Args2>
    [[nodiscard]] inline static std::shared_ptr<Object> CreateSharedPtr ( Parameter<Args1...> const & args1 = {}
        ,Parameter<Args2...> const & args2 = {} ) noexcept
    {
        try{
            return std::shared_ptr<Object>{ Create(args1,args2) };
        }catch (const std::exception &){
            return {};
        }
    }

    template<typename ...Args1,typename ...Args2>
    [[nodiscard]] inline static std::unique_ptr<Object> CreateUniquePtr ( Parameter<Args1...> const & args1 = {}
        ,Parameter<Args2...> const & args2 = {} ) noexcept
    {
        return std::unique_ptr<Object>{ Create(args1,args2) };
    }

#ifdef HAS_QT
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
};

using HelperClass [[maybe_unused]] = XHelperClass<void>;

/**
 * 辅助二阶构造,代码可能随时删除
 * @tparam Obj
 * @tparam Args1
 * @tparam Args2
 * @param args1
 * @param args2
 * @return Obj *
 */
template<typename Obj,typename ...Args1,typename ...Args2>
[[maybe_unused]] inline auto XSecondCreate( Parameter<Args1...> const & args1 = {}
    , Parameter<Args2...> const & args2 = {} ) noexcept -> Obj *
{
    return XHelperClass<Obj>::Create( args1,args2 );
}

/**
 * 辅助二阶构造,代码可能随时删除
 * @tparam Obj
 * @tparam Args1
 * @tparam Args2
 * @param args1
 * @param args2
 * @return std::unique_ptr<Obj>
 */

template<typename Obj,typename ...Args1,typename ...Args2>
[[maybe_unused]] inline auto XSecondCreateUniquePtr( Parameter<Args1...> const &args1 = {}
    ,Parameter<Args2...> const & args2 = {} ) noexcept -> std::unique_ptr<Obj>
{
    return XHelperClass<Obj>::CreateUniquePtr( args1,args2 );
}

/**
 * 辅助二阶构造,代码可能随时删除
 * @tparam Obj
 * @tparam Args1
 * @tparam Args2
 * @param args1
 * @param args2
 * @return std::shared_ptr<Obj>
 */
template<typename Obj,typename ...Args1,typename ...Args2>
[[maybe_unused]] inline auto XSecondCreateSharedPtr( Parameter<Args1...> const &args1 = {}
    ,Parameter<Args2...> const & args2  = {} ) noexcept -> std::shared_ptr<Obj>
{
    return XHelperClass<Obj>::CreateSharedPtr( args1,args2 );
}

#define X_HELPER_CLASS \
private: \
    inline void checkFriendXHelperClass_(){ X_ASSERT_W( false,FUNC_SIGNATURE \
        ,"This function is used for checking, please do not call it!"); \
    } \
    template<typename> friend class xtd::XHelperClass; \
    template<typename> friend struct xtd::XPrivate::Has_X_HELPER_CLASS_Macro; \
    template<typename ,typename ...> friend struct xtd::XPrivate::Has_construct_Func; \
    template<typename T, typename ... Args> \
    friend inline std::unique_ptr<T> xtd::make_Unique(Args && ...) noexcept; \

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
