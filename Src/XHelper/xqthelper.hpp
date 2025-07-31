#ifndef X_QT_HELPER_HPP
#define X_QT_HELPER_HPP
#ifdef HAS_QT

#include <XHelper/xhelper.hpp>
#include <type_traits>
#include <QMetaEnum>
#include <QString>
#include <QObject>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace qhelper {

#if __cplusplus >= 202002L
#define LIKE_WHICH 1
#if LIKE_WHICH == 1
    template<typename ENUM_> requires (static_cast<bool>(QtPrivate::IsQEnumHelper<ENUM_>::Value))
    inline QString getEnumTypeAndValueName(ENUM_ && enumValue) {
#elif LIKE_WHICH == 2
    template<typename ENUM_>
    inline QString getEnumTypeAndValueName(ENUM_ && enumValue)
    requires (static_cast<bool>(QtPrivate::IsQEnumHelper<ENUM_>::Value)) {
#else
    template<typename T>
    concept ENUM_T = static_cast<bool>(QtPrivate::IsQEnumHelper<T>::Value);
    template<ENUM_T ENUM_>
    inline QString getEnumTypeAndValueName(ENUM_ && enumValue) {
#endif

#else
    template<typename ENUM_>
    inline std::enable_if_t<QtPrivate::IsQEnumHelper<ENUM_>::Value, QString>
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
    inline T *findChildByName(QObject* parent, const QString& objectname) {
#elif (LIKE_WHICH == 2 )
    template<typename T>
    inline T *findChildByName(QObject* parent, const QString& objectname)
    requires(std::is_same_v<QObject,T> || std::is_base_of_v<QObject,T>) {
#else
    template<typename T>
    concept QObject_t = std::is_same_v<QObject,T> || std::is_base_of_v<QObject,T>;
    template<QObject_t T>
    inline T *findChildByName(QObject* parent, const QString& objectname) {
#endif

#else
    template <typename T>
    inline std::enable_if_t<std::disjunction_v<std::is_same<QObject,T>,std::is_base_of<QObject,T>>,T*>
    findChildByName(QObject* parent, const QString& objectname) {
#endif
        foreach (QObject* child, parent->children()) {
            if (child->objectName() == objectname) {
                return qobject_cast<T*>(child);
            }
        }
        return nullptr;
    }

    template<typename Obj>
    struct has_Friend_ConnectHelper {
    private:
        template<typename Object>
        inline static char test( void (Object::*)() ){ throw ""; }
        inline static int test( void (Obj::*)() ){ throw ""; }
    public:
        enum { value = sizeof(test(&Obj::checkFriendConnect)) == sizeof(int) };
    };

    template<typename Obj>
    inline constexpr bool has_Friend_ConnectHelper_v {has_Friend_ConnectHelper<Obj>::value};
#if 1
    template<typename Func1,typename ...Args>
    inline QMetaObject::Connection ConnectHelper(const typename QtPrivate::FunctionPointer<Func1>::Object *sender,
                                                 Func1 signal,
                                                 Args && ...args) {
        using SignalType = QtPrivate::FunctionPointer<Func1> ;
        static_assert( has_Friend_ConnectHelper_v<typename SignalType::Object>
                ,"No FRIEND_CON in the class!" );
        return QObject::connect(sender,signal,std::forward<Args>(args)...);
    }
#else
    template<typename Object,typename ...Args>
    inline QMetaObject::Connection ConnectHelper(Object * const sender,Args && ...args) {
        static_assert( has_Friend_ConnectHelper_v<Object>
                ,"No FRIEND_CON in the class!" );
        return QObject::connect(sender,std::forward<Args>(args)...);
    }
#endif

#define FRIEND_CON \
privete: \
    inline void checkFriendConnect(){ X_ASSERT_W( false,FUNC_SIGNATURE \
        ,"This function is used for checking, please do not call it!" ); }\
    template<typename ...Args>\
    friend QMetaObject::Connection xtd::qhelper::ConnectHelper(Args && ...); \
    template<typename> \
    friend struct xtd::qhelper::has_Friend_ConnectHelper;
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
#endif
