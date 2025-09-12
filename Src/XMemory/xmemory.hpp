#ifndef XUTILS_XMEMORY_HPP
#define XUTILS_XMEMORY_HPP 1

#include <XHelper/xhelper.hpp>
#include <XHelper/xqt_detection.hpp>
#ifdef HAS_QT
    #include <QMetaEnum>
    #include <QString>
    #include <QObject>
    #include <QScopedPointer>
    #include <QSharedPointer>
#endif

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace XPrivate {

    template<typename Object>
    struct Has_X_TwoPhaseConstruction_CLASS_Macro {
    private:
        static_assert(std::is_object_v<Object>,"typename Object don't Object type");

        template<typename T>
        inline static char test( void (T::*)() ){throw;}

        inline static int test( void (Object::*)() ){throw;}
    public:
        enum { value = sizeof(test(&Object::checkFriendXTwoPhaseConstruction_)) == sizeof(int) };
    };

    template<typename Object>
    inline constexpr bool Has_X_TwoPhaseConstruction_CLASS_Macro_v { Has_X_TwoPhaseConstruction_CLASS_Macro<Object>::value };

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
        -> decltype(std::declval<O>().~O(),std::false_type{})
        { throw ; }

        template<typename >
        inline static auto test(...) -> std::true_type
        { throw ; }

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

        [[maybe_unused]] static void deallocate(value_type * const ptr, std::size_t) noexcept
        { std::free(ptr); }

        friend bool operator==(Allocator_ const &,Allocator_ const &) noexcept
        { return true; }

        friend bool operator!=(const Allocator_ &, const Allocator_ &)
        { return false; }
    };

    template<typename T, typename U>
    bool operator==(const Allocator_ <T>&, const Allocator_ <U>&)
    { return true; }

    template<typename T, typename U>
    bool operator!=(const Allocator_ <T>&, const Allocator_ <U>&)
    { return false; }

#define STATIC_ASSERT_P \
    static_assert( std::is_object_v< Object >,"typename Object is not an class or struct" ); \
                        \
    static_assert( std::is_final_v< Object > ,"Object must be a final class" ); \
                        \
    static_assert( XPrivate::Has_X_TwoPhaseConstruction_CLASS_Macro_v< Object > \
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


template<typename Tp_>
class XTwoPhaseConstruction {

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
    { return {}; }

protected:
    template<typename Type> struct Destructor_ {

        using type = Type;
        using value_type = type;

        constexpr Destructor_() = default;

        template<typename U>
        [[maybe_unused]] constexpr explicit Destructor_(Destructor_<U> const &) {}

        inline static void cleanup(const value_type * const pointer) noexcept {
            static_assert(sizeof(Object) > static_cast<std::size_t>(0)
                    ,"Object must be a complete type!");
            delete pointer;
        }

        void operator()(const value_type * const pointer) const noexcept
        { cleanup(pointer); }
    };

    using Deleter = Destructor_< Object_t >;

public:
    using Object = Object_t;
    using ObjectSPtr = std::shared_ptr< Object >;
    using ObjectUPtr = std::unique_ptr< Object , Deleter >;

    template<typename ...Args1,typename ...Args2>
    [[nodiscard]]
    inline static constexpr auto Create( Parameter< Args1... > && args1 = {},
          Parameter< Args2...> && args2 = {} ) noexcept -> Object *
    {
        static_assert( std::disjunction_v< std::is_base_of< XTwoPhaseConstruction ,Object >
            ,std::is_convertible<Object,XTwoPhaseConstruction >
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
    [[nodiscard]] [[maybe_unused]]
    inline static constexpr auto CreateSharedPtr ( Parameter< Args1...> && args1 = {}
        ,Parameter< Args2...> && args2 = {} ) noexcept -> ObjectSPtr
    {
        return { Create( std::forward< decltype( args1 ) >( args1 )
            ,std::forward< decltype( args2 ) >( args2 ) ) ,Deleter{}
            ,XPrivate::Allocator_< Object >{} };
    }

    template<typename ...Args1,typename ...Args2>
    [[nodiscard]] [[maybe_unused]]
    inline static constexpr auto CreateUniquePtr ( Parameter< Args1... > && args1 = {}
        ,Parameter< Args2... > && args2 = {} ) noexcept -> ObjectUPtr
    {
        return { Create( std::forward< decltype( args1 ) >( args1 )
            ,std::forward< decltype( args2 ) >( args2 ) ) ,Deleter{} };
    }

#ifdef HAS_QT

    using ObjectQUPtr = QScopedPointer<Object,Deleter>;
    using ObjectQSPtr = QSharedPointer<Object>;

    template<typename ...Args1,typename ...Args2>
    [[nodiscard]] [[maybe_unused]]
    inline static constexpr auto CreateQScopedPointer ( Parameter< Args1... > && args1 = {}
            ,Parameter< Args2... > && args2 = {} ) noexcept -> ObjectQUPtr
    {
        return ObjectQUPtr{ Create( std::forward< decltype( args1 ) >( args1 )
                ,std::forward< decltype( args2 ) >( args2 ) ) };
    }

    template<typename ...Args1,typename ...Args2>
    [[nodiscard]] [[maybe_unused]]
    inline static constexpr auto CreateQSharedPointer ( Parameter< Args1...> && args1 = {}
            ,Parameter< Args2...> && args2 = {} ) noexcept -> ObjectQSPtr
    {
        try{
            return ObjectQSPtr { Create( std::forward< decltype( args1 ) >( args1 )
                    ,std::forward< decltype( args2 ) >( args2 ) ) ,Deleter{} };
        }catch (std::exception const &) {
            return ObjectQSPtr {};
        }
    }

   #if __cplusplus >= 202002L
   #define LIKE_WHICH 1
   #if LIKE_WHICH == 1
       template<typename ENUM_> requires (static_cast<bool>(QtPrivate::IsQEnumHelper<ENUM_>::Value))
       inline static QString getEnumTypeAndValueName(ENUM_ && enumValue) {

           if constexpr ( std::is_object_v<Object_t> ) {
               static_assert(XPrivate::Has_X_HELPER_CLASS_Macro_v<Object_t>
                       ,"No X_HELPER_CLASS in the class!");
           }

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

           if constexpr ( std::is_object_v<Object_t> ) {
               static_assert(XPrivate::Has_X_HELPER_CLASS_Macro_v<Object_t>
                       ,"No X_HELPER_CLASS in the class!");
           }

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
    XTwoPhaseConstruction() = default;
    template<typename> friend class XSingleton;
};

using TwoPhaseConstruction [[maybe_unused]] = XTwoPhaseConstruction<void>;

template<typename Tp_>
class XSingleton : protected XTwoPhaseConstruction<Tp_> {
    using Base_ = XTwoPhaseConstruction<Tp_>;
    static_assert(std::is_object_v<typename Base_::Object>,"Tp_ must be a class or struct type!");
public:
    using Object = Base_::Object;
    using SingletonPtr = Base_::ObjectSPtr;

    template<typename ...Args1,typename ...Args2>
    inline static constexpr auto UniqueConstruction([[maybe_unused]] Parameter<Args1...> && args1 = {}
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

    [[maybe_unused]] inline static constexpr auto instance() noexcept -> SingletonPtr
    { return data(); }

    [[maybe_unused]] [[nodiscard]] inline static constexpr bool isConstruct() noexcept
    { return static_cast<bool >(data()); }

#ifdef HAS_QT
    using QSingletonPtr = Base_::ObjectQSPtr;

    template<typename ...Args1,typename ...Args2>
    [[maybe_unused]] [[nodiscard]]
    inline static constexpr auto QUniqueConstruction([[maybe_unused]] Parameter<Args1...> && args1 = {}
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

    [[maybe_unused]] inline static constexpr auto qInstance() noexcept -> QSingletonPtr
    { return qdata(); }

    [[maybe_unused]] [[nodiscard]] inline static constexpr bool isQConstruct() noexcept
    { return static_cast<bool>(qdata()); }
#endif

private:
    inline static auto initFlag() noexcept -> std::once_flag &
    { static std::once_flag flag{};return flag; }

    inline static auto data() noexcept -> SingletonPtr &
    { static SingletonPtr d{}; return d; }

#ifdef HAS_QT
    inline static auto qdata() noexcept -> QSingletonPtr &
    { static QSingletonPtr d{}; return d; }
#endif

    template<typename Callable>
    inline static constexpr void Allocator_([[maybe_unused]] Callable && callable) noexcept {

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
                                , "no SingletonPtr and QSingletonPtr!");
                    }
#else
                    data().swap(ptr);
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

#define X_TWO_PHASE_CONSTRUCTION_CLASS \
private: \
    inline void checkFriendXTwoPhaseConstruction_() { X_ASSERT_W( false ,FUNC_SIGNATURE \
        ,"This function is used for checking, please don't call it!"); \
    } \
    template<typename> friend class XUtils::XTwoPhaseConstruction; \
    template<typename> friend struct XUtils::XPrivate::Has_X_TwoPhaseConstruction_CLASS_Macro; \
    template<typename ,typename ...> friend struct XUtils::XPrivate::Has_construct_Func; \
    template<typename> friend class XUtils::XSingleton;

#if __cplusplus >= 201402L

template< typename T,typename ...Args,typename Ret = std::shared_ptr<T>>
[[maybe_unused]] [[nodiscard]]
inline auto makeShared(Args && ...args) noexcept
    -> std::enable_if_t<std::negation_v<std::is_array<T>>,Ret>
{
    try{
        return std::allocate_shared<T>(std::allocator<T>{} , std::forward<Args>(args)... );
    }catch (const std::exception &){
        return Ret {};
    }
}

template<typename T ,typename Ret = std::shared_ptr<T>>
[[maybe_unused]] [[nodiscard]]
inline auto makeShared(std::size_t const n) noexcept
    -> std::enable_if_t<std::is_unbounded_array_v<T>,Ret>
{
    try{
        return std::allocate_shared<T>(std::allocator<T>{},n);
    }catch (const std::exception &){
        return Ret {};
    }
}

template<typename T ,typename Ret = std::shared_ptr<T>>
[[maybe_unused]] [[nodiscard]]
inline auto makeShared() noexcept
    -> std::enable_if_t<std::is_bounded_array_v<T>,Ret>
{
    try{
        return std::allocate_shared<T>(std::allocator<T>{});
    }catch (const std::exception &){
        return Ret {};
    }
}

template<typename T ,typename Ret = std::shared_ptr<T>>
[[maybe_unused]] [[nodiscard]]
inline auto makeShared(std::size_t const n,const std::remove_extent_t<T> & u ) noexcept
    -> std::enable_if_t<std::is_unbounded_array_v<T>,Ret>
{
    try{
        return std::allocate_shared<T>(std::allocator<T>{},n,u);
    }catch (const std::exception &){
        return Ret {};
    }
}

template<typename T ,typename Ret = std::shared_ptr<T>>
[[maybe_unused]] [[nodiscard]]
inline auto makeShared(std::remove_extent_t<T> const & u ) noexcept
    -> std::enable_if_t<std::is_bounded_array_v<T>,Ret>
{
    try{
        return std::allocate_shared<T>(std::allocator<T>{},u);
    }catch (const std::exception &){
        return Ret {};
    }
}

template<typename T,typename Ret = std::unique_ptr<T> >
[[maybe_unused]] [[nodiscard]]
inline auto makeUnique(std::size_t const n) noexcept
    -> std::enable_if_t<std::is_array_v<T> && !std::extent_v<T>, Ret>
{
    try {
        return std::make_unique<T>(n);
    } catch (std::exception const &) {
        return Ret {};
    }
}

template<typename T,typename ...Args,std::enable_if_t<std::extent_v<T> != 0,int> = 0>
void makeUnique(Args && ...) noexcept = delete;

#endif

#define MAKE_POINTER_FUNC(funcName,type) \
    template<typename T,typename ...Args,typename Ret = type<T> > \
    [[maybe_unused]] [[nodiscard]] \
    inline auto funcName (Args && ...args) noexcept \
        -> std::enable_if_t< !std::is_array_v<T> , Ret> { \
        try{ return Ret { new T( std::forward<Args>(args)... ) };} \
        catch (const std::exception &) {return Ret {};}  \
    }

    MAKE_POINTER_FUNC(makeUnique,std::unique_ptr)

#ifdef HAS_QT
    MAKE_POINTER_FUNC(makeQScoped,QScopedPointer)
    MAKE_POINTER_FUNC(makeQShared,QSharedPointer)
#endif

#undef MAKE_POINTER_FUNC

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
