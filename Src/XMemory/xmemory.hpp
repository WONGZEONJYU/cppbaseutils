#ifndef XUTILS_XMEMORY_HPP
#define XUTILS_XMEMORY_HPP 1

#include <thread>
#include <mutex>
#include <sstream>
#include <XHelper/xhelper.hpp>
#include <XHelper/xqt_detection.hpp>
#include <XAtomic/xatomic.hpp>
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

    template<typename Tuple_> requires (is_tuple_v<Tuple_>)
    static constexpr auto indices(Tuple_ &&) noexcept
        -> std::make_index_sequence< std::tuple_size_v< std::decay_t< Tuple_ > > >
    { return {}; }

    template<typename Tuple_> requires (is_tuple_v<Tuple_>)
    static constexpr auto indices() noexcept
        -> std::make_index_sequence< std::tuple_size_v< std::decay_t< Tuple_ > > >
    { return {}; }

    template<typename Object>
    struct Has_X_TwoPhaseConstruction_CLASS_Macro {
    private:
        static_assert(std::is_object_v<Object>,"typename Object don't Object type");

        template<typename T>
        static constexpr char test( void (T::*)() ) noexcept { return {}; }

        static constexpr int test( void (Object::*)() ) noexcept { return {}; }

    public:
        enum { value = sizeof(test(&Object::checkFriendXTwoPhaseConstruction_)) == sizeof(int) };
    };

    template<typename Object>
    inline constexpr bool Has_X_TwoPhaseConstruction_CLASS_Macro_v { Has_X_TwoPhaseConstruction_CLASS_Macro<Object>::value };

    template<typename Object,typename Tuple>
    struct Has_construct_Func {
    private:
        static_assert(std::is_object_v<Object>,"typename Object don't Object type");
        static_assert(is_tuple_v<Tuple>,"typename Tuple don't std::tuple type");

        template<typename O,typename Tuple_,std::size_t ...I>
        static constexpr auto test(std::index_sequence<I...> ) noexcept -> std::true_type
            requires ( ( sizeof( std::declval<O>().construct_( ( std::declval< std::decay_t< std::tuple_element_t<I,Tuple_> > >() )... ) )
                > static_cast<std::size_t>(0) ) )
        { return {};}

        template<typename ...>
        static constexpr auto test(...) noexcept -> std::false_type
        { return {}; }

        using decayedTuple_ = std::decay_t< Tuple >;

    public:
        enum { value = decltype(test<Object,decayedTuple_>(indices<Tuple>()))::value };
    };

    template<typename ...Args>
    inline constexpr bool Has_construct_Func_v {Has_construct_Func<Args...>::value};

    template<typename Object,typename Tuple>
    struct is_private_mem_func {
    private:
        static_assert(std::is_object_v<Object>,"typename Object don't Object type");
        static_assert(is_tuple_v<Tuple>,"typename Tuple don't std::tuple type");

        template<typename O,typename Tuple_,std::size_t ...I>
        static constexpr auto test(std::index_sequence<I...>) noexcept -> std::false_type
            requires (
                ( sizeof( std::declval<O>().construct_( std::declval< std::decay_t< std::tuple_element_t<I,Tuple_> > >()...) ) > static_cast<std::size_t>(0) )
                    || std::is_same_v< decltype( std::declval<O>().construct_( std::declval< std::decay_t< std::tuple_element_t<I,Tuple_> > >()...) ),void >
            )
        { return {}; }

        template<typename ...>
        static constexpr auto test(...) noexcept -> std::true_type
        { return {}; }

        using decayedTuple_ = std::decay_t< Tuple >;

    public:
        enum { value = decltype(test<Object,decayedTuple_>(indices<Tuple>()))::value };
    };

    template<typename ...Args>
    inline constexpr bool is_private_mem_func_v{ is_private_mem_func<Args...>::value };

    template<typename Object, typename Tuple>
    struct is_default_constructor_accessible {
    private:
        static_assert(std::is_object_v<Object>,"typename Object don't Object type");
        static_assert(is_tuple_v<Tuple>,"typename Tuple don't std::tuple type");

        template<typename O,typename Tuple_,std::size_t ...I>
        static constexpr auto result(std::index_sequence<I...>) noexcept {
            return std::disjunction_v< std::is_constructible< O, std::decay_t< std::tuple_element_t<I,Tuple_> >... >
                    ,std::is_nothrow_constructible< O ,std::decay_t< std::tuple_element_t<I,Tuple_> >... >
                    ,std::is_trivially_constructible< O ,std::decay_t< std::tuple_element_t<I,Tuple_> >... >
            >;
        }

        template<typename > struct is_copy_move_constructor
        { enum { value = false }; };

        template<typename Tuple_> requires(std::tuple_size_v<Tuple_> == 1)
        struct is_copy_move_constructor<Tuple_> {
            using First_ = std::tuple_element_t<0, Tuple_>;
            enum {
                value = std::disjunction_v<
                    std::is_same<First_, Object &>,
                    std::is_same<First_, const Object &>,
                    std::is_same<First_, Object &&>,
                    std::is_same<First_, const Object &&>
                >
            };
        };

        using decayedTuple_ = std::decay_t< Tuple >;

    public:
        enum {value = result<Object,decayedTuple_>(indices<Tuple>()) && !is_copy_move_constructor<Tuple>::value};
    };

    template<typename ...Args>
    inline constexpr bool is_default_constructor_accessible_v { is_default_constructor_accessible<Args...>::value };

    template<typename Object>
    struct is_destructor_private {
    private:
        static_assert(std::is_object_v<Object>,"typename Object don't Object type");

        template<typename O>
        static constexpr auto test(int) noexcept -> decltype(std::declval<O>().~O(),std::false_type{})
        { return {}; }

        template<typename >
        static constexpr auto test(...) noexcept -> std::true_type
        { return {}; }

    public:
        enum {value = decltype(test<Object>(0))::value };
    };

    template<typename Object>
    inline constexpr bool is_destructor_private_v { is_destructor_private<Object>::value };

    template<typename Tp_>
    struct Allocator_ {
    private:
        template<typename > friend struct Allocator_;

        static constexpr auto DEFAULT_WAIT_TIME { 100000ULL };
        static constexpr auto DEFAULT_RETRY_COUNT { 5UL };

        mutable std::atomic<std::chrono::nanoseconds> m_waitTime_ {
            std::chrono::nanoseconds {DEFAULT_WAIT_TIME}
        };

        mutable XAtomicInteger<std::size_t> m_retryCount_{DEFAULT_RETRY_COUNT};

    public:
        using value_type = Tp_;

        constexpr Allocator_() = default;

        template<typename U>
        constexpr Allocator_(Allocator_<U> const & o) noexcept {
            m_waitTime_.store(o.m_waitTime_.load(),std::memory_order_release);
            m_retryCount_.storeRelease(o.m_retryCount_.loadAcquire());
        }

        template<typename U>
        constexpr Allocator_ & operator=(Allocator_<U> const & o) noexcept {
            m_waitTime_.store(o.m_waitTime_.load(),std::memory_order_release);
            m_retryCount_.storeRelease(o.m_retryCount_.loadAcquire());
            return *this;
        }

        constexpr Allocator_(Allocator_ const & o) noexcept {
            m_waitTime_.store(o.m_waitTime_.load(),std::memory_order_release);
            m_retryCount_.storeRelease(o.m_retryCount_.loadAcquire());
        }

        constexpr Allocator_ & operator=(Allocator_ const & o) noexcept {
            m_waitTime_.store(o.m_waitTime_.load(),std::memory_order_release);
            m_retryCount_.storeRelease(o.m_retryCount_.loadAcquire());
            return *this;
        }

        template<typename Rep_,typename Period_>
        constexpr void setWaitTime(std::chrono::duration<Rep_,Period_> const & waitTime) const noexcept
        { m_waitTime_.store(std::chrono::duration_cast<std::chrono::nanoseconds>(waitTime),std::memory_order_release) ; }

        constexpr void setRetryCount(std::size_t const retryCount) const noexcept
        { m_retryCount_.storeRelaxed(retryCount); }

        constexpr auto allocate(std::size_t const n) const noexcept -> value_type * {
            auto const retryCount{ m_retryCount_.loadAcquire() };
            auto const waitTime{ m_waitTime_.load(std::memory_order_acquire) };
            for (std::size_t i {}; i < retryCount ; ++i) {
                try {
                    if (auto const ptr{ operator new (sizeof(value_type) * n) }) {
                        return static_cast< value_type * >(ptr);
                    }
                } catch (std::exception const &) {}
                std::this_thread::sleep_for(waitTime);
            }
            return nullptr;
        }

        static constexpr void deallocate(value_type * const ptr, std::size_t const length) noexcept
        { ::operator delete(ptr,length); }

        static constexpr void deallocate(value_type * const ptr) noexcept
        { ::operator delete(ptr); }
    };

    template<typename T, typename U>
    constexpr bool operator==(const Allocator_ <T>&, const Allocator_ <U>&) noexcept
    { return true; }

    template<typename T, typename U>
    constexpr bool operator!=(const Allocator_ <T>&, const Allocator_ <U>&) noexcept
    { return false; }

#define STATIC_ASSERT_P \
    static_assert( std::is_object_v< Object >,"typename Object is not an class or struct" ); \
                        \
    static_assert( std::is_final_v< Object > ,"Object must be a final class" ); \
                        \
    static_assert( XPrivate::Has_X_TwoPhaseConstruction_CLASS_Macro_v< Object > \
            ,"No X_HELPER_CLASS in the class!" ); \
                        \
    static_assert( XPrivate::Has_construct_Func_v< Object ,ArgsList2 > \
            ,"bool Object::construct_(...) non static member function absent!" ); \
                        \
    static_assert( XPrivate::is_private_mem_func_v< Object ,ArgsList2 > \
            ,"bool Object::construct_(...) must be a private non static member function!" ); \
                        \
    static_assert( !XPrivate::is_default_constructor_accessible_v< Object ,ArgsList1 > \
            ,"The Object (...) constructor (non copy and non move) must be a private member function!" );

} //namespace XPrivate;

template<typename Tp_, typename = XPrivate::Allocator_< std::decay_t< RemoveRef_T<Tp_> > > >
class XTwoPhaseConstruction;

template<typename Tp_, typename = XPrivate::Allocator_< std::decay_t< RemoveRef_T< Tp_ > > > >
class XSingleton;

template<typename ...Args>
using Parameter = std::tuple<Args...>;

template<typename ...Args>
using XArgs = std::tuple<Args...>;

template<typename Tp_, typename Alloc_>
class XTwoPhaseConstruction {

    using Object_t = std::decay_t< RemoveRef_T<Tp_> >;
    using Allocator = Alloc_;

    static_assert(std::negation_v< std::is_pointer< Object_t > >,"Tp_ Cannot be pointer type");

    inline static Allocator sm_allocator_{};

protected:
    template<typename Type> struct Destructor_ {

        using type = Type;
        using value_type = type;
        using allocator_type = Allocator;

        constexpr Destructor_() = default;

        template<typename U > requires (std::is_constructible_v<U*,value_type *>)
        constexpr Destructor_(Destructor_<U> const &) {}

        static constexpr void cleanup(value_type * const pointer) noexcept {
            static_assert(sizeof(Object_t) > static_cast<std::size_t>(0)
                    ,"Object must be a complete type!");
            if (!pointer) { return; }
            // 先调用析构函数
            pointer->~value_type();
            // 然后使用默认分配器释放内存 (静态函数无法访问成员分配器)
            std::allocator_traits<allocator_type>::deallocate(sm_allocator_, pointer, 1);
        }

        constexpr void operator()(value_type * const pointer) const noexcept
        { cleanup(pointer); }
    };

    using Deleter = Destructor_< Object_t >;

public:
    using Object = Object_t;
    using ObjectSPtr = std::shared_ptr< Object >;
    using ObjectUPtr = std::unique_ptr< Object , Deleter >;

    static constexpr auto & getAllocator() noexcept
    { return sm_allocator_; }

    // 为裸指针提供专门的删除函数
    static constexpr void Delete(Object * const pointer) noexcept {
        if (!pointer) { return; }
        pointer->~Object();
        std::allocator_traits<Allocator>::deallocate(sm_allocator_, pointer, 1);
    }
    // 重写operator delete以支持Qt对象树
    // 这是更安全的方法，让Qt调用我们自定义的delete操作符
    constexpr void operator delete(void * const ptr, std::size_t const length) noexcept {
        if (!ptr || !length) { return; }
        std::allocator_traits<Allocator>::deallocate(sm_allocator_,static_cast<Object *>(ptr),length);
    }

    constexpr void operator delete(void * const ptr) noexcept
    { operator delete(ptr,1); }

#ifdef HAS_QT

    // Qt对象树专用创建方法 - 返回裸指针供Qt对象树管理
    // 如果父指针为null,即使创建成功也会马上销毁并返回null,请确保父指针不能为null
    template<typename QtObjectPointer,typename ArgsList1 = Parameter<> , typename ArgsList2 = Parameter<> >
        requires(std::conjunction_v<is_tuple<ArgsList1> , is_tuple<ArgsList2>>)
    [[nodiscard]]
    static constexpr auto CreateForQtObjectTree(QtObjectPointer * const parent,ArgsList1 && args1 = {}, ArgsList2 && args2 = {})
        noexcept -> Object *
    {
        static_assert(std::is_base_of_v<QObject, Object> ,
                     "Object must inherit from QObject to be used in Qt Object tree");

        auto obj { CreateUniquePtr(std::forward<decltype(args1)>(args1), std::forward<decltype(args2)>(args2)) };

        // 设置父对象,让Qt对象树管理生命周期
        return obj && parent ? obj->setParent(parent),obj.release() : nullptr;
    }

    template<typename QtObjectPointer,typename ArgsList1 = Parameter<> , typename ArgsList2 = Parameter<> >
        requires(std::conjunction_v<is_tuple<ArgsList1> , is_tuple<ArgsList2>>)
    [[nodiscard]]
    static constexpr auto CreateForQtObjectTree(QtObjectPointer && parent,ArgsList1 && args1 = {}, ArgsList2 && args2 = {})
        noexcept -> Object *
    {
        return CreateForQtObjectTree(parent.get()
            ,std::forward<decltype(args1)>(args1)
            ,std::forward<decltype(args2)>(args2));
    }

    template<typename QtObject,typename ArgsList1 = Parameter<> , typename ArgsList2 = Parameter<> >
        requires(std::conjunction_v<is_tuple<ArgsList1> , is_tuple<ArgsList2>>)
    [[nodiscard]]
    static constexpr auto CreateForQtObjectTree(QtObject & parent,ArgsList1 && args1 = {}, ArgsList2 && args2 = {})
        noexcept -> Object *
    {
        return CreateForQtObjectTree(std::addressof(parent)
            ,std::forward<decltype(args1)>(args1)
            ,std::forward<decltype(args2)>(args2));
    }

    // 为Qt对象提供手动从对象树中移除并删除的方法
    template<typename QtObjectPointer>
    static constexpr void DeleteFromQtObjectTree(QtObjectPointer * const pointer) noexcept {
        if (!pointer) { return; }
        // 先从父对象中移除，避免Qt自动删除
        if (pointer->parent()) { pointer->setParent(nullptr); }
        // 然后使用我们的删除方法
        Delete(reinterpret_cast<Object*>(pointer));
    }

    // Qt对象树兼容的内存释放器 - 只释放内存,不调用析构函数
    // 注意:这个方法只能在对象已经被析构后调用
    template<typename QtObjectPointer>
    static constexpr void QtCompatibleDeallocate(QtObjectPointer * const pointer) noexcept {
        if (!pointer) { return; }
        // 只释放内存,不调用析构函数(析构函数已经被调用了)
        std::allocator_traits<Allocator>::deallocate(sm_allocator_, reinterpret_cast<Object *>(pointer), 1);
    }

#endif

    template< typename ArgsList1 = Parameter<> ,typename ArgsList2 = Parameter<> >
        requires(std::conjunction_v<is_tuple<ArgsList1> , is_tuple<ArgsList2>>)
    static constexpr auto Create( ArgsList1 && args1 = {},ArgsList2 && args2 = {} )
        noexcept -> Object *
    {
        static_assert( std::disjunction_v< std::is_base_of< XTwoPhaseConstruction ,Object >
            ,std::is_convertible<Object,XTwoPhaseConstruction >
        > ,"Object must inherit from Class XTwoPhaseConstruction" );

        STATIC_ASSERT_P

        return [&]< std::size_t ...I1 ,std::size_t...I2 >( std::index_sequence< I1... > ,std::index_sequence< I2... > )
            noexcept -> Object *
        {
            try{
                auto const raw_ptr { std::allocator_traits<Allocator>::allocate( sm_allocator_, 1) };
                auto const obj_ptr { new (raw_ptr) Object( std::get<I1>( std::forward< decltype( args1 ) >( args1 ) )... ) };
                ObjectUPtr obj { obj_ptr, Deleter {} };
                return obj_ptr && obj->construct_( std::get<I2>( std::forward< decltype( args2 ) >( args2 ) )... ) ? obj.release() : nullptr;
            } catch (const std::exception &) {
                return nullptr;
            }
        }( XPrivate::indices( args1 ) ,XPrivate::indices( args2 ) );
    }

    template< typename ArgsList1 = Parameter<> ,typename ArgsList2 = Parameter<> >
        requires(std::conjunction_v<is_tuple<ArgsList1> , is_tuple<ArgsList2>>)
    static constexpr auto CreateUniquePtr ( ArgsList1 && args1 = {},ArgsList2 && args2 = {} )
        noexcept -> ObjectUPtr
    {
        return { Create( std::forward< decltype( args1 ) >( args1 )
            ,std::forward< decltype( args2 ) >( args2 ) ) ,Deleter {} };
    }

    template<typename ArgsList1 = Parameter<> ,typename ArgsList2 = Parameter<> >
        requires(std::conjunction_v<is_tuple<ArgsList1> , is_tuple<ArgsList2>>)
    static constexpr auto CreateSharedPtr ( ArgsList1 && args1 = {} ,ArgsList2 && args2 = {} )
        noexcept -> ObjectSPtr
    {
        return ObjectSPtr { Create( std::forward< decltype( args1 ) >( args1 )
            ,std::forward< decltype( args2 ) >( args2 ) ) ,Deleter{} , sm_allocator_ };
    }

#ifdef HAS_QT

    using ObjectQUPtr = QScopedPointer<Object,Deleter>;
    using ObjectQSPtr = QSharedPointer<Object>;

    template<typename ArgsList1 = Parameter<> ,typename ArgsList2 = Parameter<> >
        requires(std::conjunction_v<is_tuple<ArgsList1> , is_tuple<ArgsList2>>)
    [[nodiscard]] [[maybe_unused]]
    static constexpr auto CreateQScopedPointer ( ArgsList1 && args1 = {},ArgsList2 && args2 = {} )
        noexcept -> ObjectQUPtr
    {
        return ObjectQUPtr{ Create( std::forward< decltype( args1 ) >( args1 )
                ,std::forward< decltype( args2 ) >( args2 ) ) };
    }

    template<typename ArgsList1 = Parameter<> ,typename ArgsList2 = Parameter<> >
        requires(std::conjunction_v<is_tuple<ArgsList1> , is_tuple<ArgsList2>>)
    [[nodiscard]] [[maybe_unused]]
    static constexpr auto CreateQSharedPointer ( ArgsList1 && args1 = {},ArgsList2 && args2 = {} )
        noexcept -> ObjectQSPtr
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
       static constexpr QString getEnumTypeAndValueName(ENUM_ && enumValue) {

           if constexpr ( std::is_object_v<Object_t> ) {
               static_assert(XPrivate::Has_X_TwoPhaseConstruction_CLASS_Macro_v<Object_t>
                       ,"No X_HELPER_CLASS in the class!");
           }

   #elif LIKE_WHICH == 2
       template<typename ENUM_>
       static constexpr QString getEnumTypeAndValueName(ENUM_ && enumValue)
       requires (static_cast<bool>(QtPrivate::IsQEnumHelper<ENUM_>::Value)) {
   #else
       template<typename T>
       concept ENUM_T = static_cast<bool>(QtPrivate::IsQEnumHelper<T>::Value);
       template<ENUM_T ENUM_>
       static constexpr QString getEnumTypeAndValueName(ENUM_ && enumValue) {
   #endif
   #else
       template<typename ENUM_>
       static constexpr std::enable_if_t<QtPrivate::IsQEnumHelper<ENUM_>::Value, QString>
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
       static constexpr T * findChildByName(QObject* parent, const QString& objectname) {

           if constexpr ( std::is_object_v<Object_t> ) {
               static_assert(XPrivate::Has_X_TwoPhaseConstruction_CLASS_Macro_v<Object_t>
                       ,"No X_HELPER_CLASS in the class!");
           }

   #elif (LIKE_WHICH == 2 )
       template<typename T>
       static constexpr T * findChildByName(QObject* parent, const QString& objectname)
       requires(std::is_same_v<QObject,T> || std::is_base_of_v<QObject,T>) {
   #else
       template<typename T>
       concept QObject_t = std::is_same_v<QObject,T> || std::is_base_of_v<QObject,T>;
       template<QObject_t T>
       static constexpr T *findChildByName(QObject* parent, const QString& objectname) {
   #endif
   #else
       template <typename T>
       static constexpr std::enable_if_t<std::disjunction_v<std::is_same<QObject,T>,std::is_base_of<QObject,T>>,T*>
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
       static constexpr QMetaObject::Connection ConnectHelper(Args && ...args) {
           if constexpr ( std::is_object_v<Object_t> ) {
               static_assert(XPrivate::Has_X_TwoPhaseConstruction_CLASS_Macro_v<Object_t>
                       ,"No X_HELPER_CLASS in the class!");
           }
           return QObject::connect(std::forward<Args>(args)...);
       }
#endif

protected:
    constexpr XTwoPhaseConstruction() = default;
    template<typename ,typename > friend class XSingleton;
};

using TwoPhaseConstruction = XTwoPhaseConstruction<void>;
using TPC = TwoPhaseConstruction;

template<typename Tp_, typename Alloc_ >
class XSingleton : protected XTwoPhaseConstruction<Tp_, Alloc_> {
    using Base_ = XTwoPhaseConstruction<Tp_, Alloc_>;
    static_assert(std::is_object_v<typename Base_::Object>,"Tp_ must be a class or struct type!");

public:
    using Object = Base_::Object;
    using SingletonPtr = Base_::ObjectSPtr;

    template< typename ArgsList1 = Parameter<>,typename ArgsList2 = Parameter<> >
        requires(std::conjunction_v<is_tuple<ArgsList1> , is_tuple<ArgsList2>>)
    static constexpr auto UniqueConstruction(ArgsList1 && args1 = {}, ArgsList2 && args2 = {})
        noexcept -> SingletonPtr
    {
        static_assert( XPrivate::is_destructor_private_v< Object >
                , "destructor( ~Object() ) must be private!" );

        static_assert( std::disjunction_v< std::is_base_of< XSingleton ,Object >
                ,std::is_convertible<Object,XSingleton >
        > ,"Object must inherit from Class XSingleton" );

        STATIC_ASSERT_P

        allocate_([&args1,&args2]()noexcept {
            return Base_::CreateSharedPtr(std::forward< decltype(args1) >(args1)
                    ,std::forward<decltype(args2) >(args2));
        });

        return data();
    }

    [[maybe_unused]] static constexpr auto instance() noexcept -> SingletonPtr
    { return data(); }

    [[maybe_unused]] [[nodiscard]] static constexpr bool isConstruct() noexcept
    { return static_cast<bool >(data()); }

#ifdef HAS_QT
    using QSingletonPtr = Base_::ObjectQSPtr;

    template< typename ArgsList1 = Parameter<>,typename ArgsList2 = Parameter<> >
        requires(std::conjunction_v<is_tuple<ArgsList1> , is_tuple<ArgsList2>>)
    [[maybe_unused]] [[nodiscard]]
    static constexpr auto QUniqueConstruction(ArgsList1 && args1 = {}, ArgsList2 && args2 = {})
        noexcept -> QSingletonPtr
    {
        static_assert( XPrivate::is_destructor_private_v< Object >
                , "destructor( ~Object() ) must be private or protected!" );

        static_assert( std::disjunction_v<
                std::conjunction< std::is_base_of< QObject ,Object >
                ,std::is_base_of< XSingleton ,Object > >
                ,std::is_convertible<Object,XSingleton >
        > ,"Object must inherit from Class QObject and Class XSingleton!" );

        STATIC_ASSERT_P

        allocate_([&args1,&args2]()noexcept{
            return Base_::CreateQSharedPointer( std::forward< decltype( args1 ) >( args1 )
                    ,std::forward< decltype( args2 ) >( args2 ) );
        });
        return qdata();
    }

    [[maybe_unused]] static constexpr auto qInstance() noexcept -> QSingletonPtr
    { return qdata(); }

    [[maybe_unused]] [[nodiscard]] static constexpr bool isQConstruct() noexcept
    { return static_cast<bool>(qdata()); }
#endif

private:
    static constexpr auto initFlag() noexcept -> std::once_flag &
    { static std::once_flag flag{};return flag; }

    static constexpr auto data() noexcept -> SingletonPtr &
    { static SingletonPtr d{}; return d; }

#ifdef HAS_QT
    static constexpr auto qdata() noexcept -> QSingletonPtr &
    { static QSingletonPtr d{}; return d; }
#endif

    template<typename Callable>
    static constexpr void allocate_([[maybe_unused]] Callable && callable) noexcept {

        std::call_once(initFlag(),[&callable]{

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
    constexpr XSingleton() = default;
    X_DISABLE_COPY_MOVE(XSingleton)
};

#undef STATIC_ASSERT_P

#define X_TWO_PHASE_CONSTRUCTION_CLASS \
private: \
    inline void checkFriendXTwoPhaseConstruction_() { X_ASSERT_W( false ,FUNC_SIGNATURE \
        ,"This function is used for checking, please don't call it!"); \
    } \
    template<typename,typename > friend class XUtils::XTwoPhaseConstruction; \
    template<typename> friend struct XUtils::XPrivate::Has_X_TwoPhaseConstruction_CLASS_Macro; \
    template<typename ,typename > friend struct XUtils::XPrivate::Has_construct_Func; \
    template<typename,typename > friend class XUtils::XSingleton;

#if __cplusplus >= 201402L

template< typename T,typename ...Args,typename Ret = std::shared_ptr<T>>
constexpr auto makeShared(Args && ...args) noexcept
    -> std::enable_if_t<std::negation_v<std::is_array<T>>,Ret>
{
    try{
        return std::allocate_shared<T>(std::allocator<T>{} , std::forward<Args>(args)... );
    }catch (const std::exception &){
        return Ret {};
    }
}

template<typename T ,typename Ret = std::shared_ptr<T>>
constexpr auto makeShared(std::size_t const n) noexcept
    -> std::enable_if_t<std::is_unbounded_array_v<T>,Ret>
{
    try{
        return std::allocate_shared<T>(std::allocator<T>{},n);
    }catch (const std::exception &){
        return Ret {};
    }
}

template<typename T ,typename Ret = std::shared_ptr<T>>
constexpr auto makeShared() noexcept
    -> std::enable_if_t<std::is_bounded_array_v<T>,Ret>
{
    try{
        return std::allocate_shared<T>(std::allocator<T>{});
    }catch (const std::exception &){
        return Ret {};
    }
}

template<typename T ,typename Ret = std::shared_ptr<T>>
constexpr auto makeShared(std::size_t const n,const std::remove_extent_t<T> & u ) noexcept
    -> std::enable_if_t<std::is_unbounded_array_v<T>,Ret>
{
    try{
        return std::allocate_shared<T>(std::allocator<T>{},n,u);
    }catch (const std::exception &){
        return Ret {};
    }
}

template<typename T ,typename Ret = std::shared_ptr<T>>
constexpr auto makeShared(std::remove_extent_t<T> const & u ) noexcept
    -> std::enable_if_t<std::is_bounded_array_v<T>,Ret>
{
    try{
        return std::allocate_shared<T>(std::allocator<T>{},u);
    }catch (const std::exception &){
        return Ret {};
    }
}

template<typename T,typename Ret = std::unique_ptr<T> >
constexpr auto makeUnique(std::size_t const n) noexcept
    -> std::enable_if_t<std::is_array_v<T> && !std::extent_v<T>, Ret>
{
    try {
        return std::make_unique<T>(n);
    } catch (std::exception const &) {
        return Ret {};
    }
}

template<typename T,typename ...Args>
constexpr auto makeUnique(Args && ...) noexcept -> std::enable_if_t<std::extent_v<T> != 0> = delete;

#endif

#define MAKE_POINTER_FUNC(funcName,type) \
    template<typename T,typename ...Args,typename Ret = type<T> > \
    constexpr auto funcName (Args && ...args) noexcept \
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
