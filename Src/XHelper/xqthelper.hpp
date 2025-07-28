#ifndef X_QT_HELPER_HPP
#define X_QT_HELPER_HPP
#ifdef HAS_QT

#include <XHelper/xversion.hpp>
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

    template<typename ...Args>
    inline QMetaObject::Connection ConnectHelper(Args && ...args) {
        return QObject::connect(std::forward<Args>(args)...);
    }

#define FRIEND_CON \
    template<typename ...Args>\
    friend QMetaObject::Connection xtd::qhelper::ConnectHelper(Args && ...args);

}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
#endif
