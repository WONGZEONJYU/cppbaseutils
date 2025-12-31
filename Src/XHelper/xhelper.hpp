#ifndef X_HELPER_HPP_
#define X_HELPER_HPP_

#include <XHelper/xversion.hpp>
#include <XHelper/xtypetraits.hpp>
#include <XGlobal/xclasshelpermacros.hpp>
#include <XHelper/xqt_detection.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <functional>
#include <iostream>
#include <chrono>
#include <ranges>
#include <algorithm>
#ifdef HAS_QT
    #include <QMetaEnum>
    #include <QString>
    #include <QObject>
    #include <QScopedPointer>
    #include <QSharedPointer>
#endif

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

#define TO_STRING(x) #x

template<typename T,typename > class XTwoPhaseConstruction;
template<typename T,typename > class XSingleton;

class X_CLASS_EXPORT XUtilsLibErrorLog final {
    constexpr XUtilsLibErrorLog() = default;
    static void log(std::string_view const & );
    template<typename ,typename > friend class XSingleton;
    template<typename ,typename > friend class XTwoPhaseConstruction;
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

X_API void consoleOut(std::string const &) noexcept;

#if 0
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

#endif

template<typename T>
concept StandardChar =
    std::same_as<T, char>     ||
    std::same_as<T, wchar_t>  ||
    std::same_as<T, char8_t>  ||
    std::same_as<T, char16_t> ||
    std::same_as<T, char32_t>;

template<typename T>
concept WritableCharRange =
    std::ranges::range<T> &&
    StandardChar<std::ranges::range_value_t<T>> &&
    requires(T & t) { *t.begin() = typename T::value_type{}; };

template<
    std::ranges::range Range,
    typename Alloc = std::allocator<std::ranges::range_value_t<Range>>
>
requires StandardChar<std::ranges::range_value_t<Range>>
constexpr auto toLower(Range && r, const Alloc & = Alloc{}){
    using CharT = std::ranges::range_value_t<Range>;

    if constexpr (WritableCharRange<Range>) {
        // 可写 → 就地修改
        std::ranges::transform(r, r.begin()
            ,[]<typename T0>(T0 const ch) { return static_cast<T0>(std::tolower(ch)); });
        return std::forward<Range>(r);
    } else {
        // 不可写 → 返回新的 basic_string
        std::basic_string<CharT, std::char_traits<CharT>, Alloc> result{};
        if constexpr (std::ranges::sized_range<Range>)
        { result.reserve(std::ranges::size(r)); }

        std::ranges::transform(r, std::back_inserter(result),
            []<typename T0>(T0 const ch) { return static_cast<T0>(std::tolower(ch)); });
        return result;
    }
}

template<
    std::ranges::range Range,
    typename Alloc = std::allocator<std::ranges::range_value_t<Range>>
>
requires StandardChar<std::ranges::range_value_t<Range>>
constexpr auto toUpper(Range && r, const Alloc & = Alloc{}){
    using CharT = std::ranges::range_value_t<Range>;

    if constexpr (WritableCharRange<Range>) {
        // 可写 → 就地修改
        std::ranges::transform(r, r.begin()
            ,[]<typename T0>(T0 const ch) { return static_cast<T0>(std::toupper(ch)); });
        return std::forward<Range>(r);
    } else {
        // 不可写 → 返回新的 basic_string
        std::basic_string<CharT, std::char_traits<CharT>, Alloc> result {};
        if constexpr (std::ranges::sized_range<Range>)
        { result.reserve(std::ranges::size(r)); }

        std::ranges::transform(r, std::back_inserter(result)
            ,[]<typename T0>(T0 const ch) { return static_cast<T0>(std::toupper(ch)); });
        return result;
    }
}

#if 0
enum class ConnectionType {
    AutoConnection,
    DirectConnection,
    QueuedConnection,
    BlockingQueuedConnection,
    UniqueConnection
};
#endif

enum class NonConst{};
enum class Const{};

#ifdef HAS_QT

#define QTR(x) QObject::tr(x)

#define CHECK_BASE(cond,x) do { \
    if((cond)) {break;} \
    auto _msg_{ QTR("file: ") }; \
    _msg_+= QTR(__FILE__) + QTR(" , ") + QTR("line: ") \
    + QString::number(__LINE__) + QTR(" , ") + QTR(FUNC_SIGNATURE) + QTR(" ")  \
    + QTR(#cond) + QTR(" ") + QTR((x)); \
    XUtils::consoleOut(_msg_.toStdString()); \
    return {};\
}while(false)

#else

#define CHECK_BASE(cond,x) do { \
    if((cond)) {break;} \
    std::ostringstream _msg_{}; \
    _msg_ << "file: " << __FILE__ << " , " \
        << "line: " << __LINE__ << " , " << FUNC_SIGNATURE << " " \
        << #cond << " " << (x); \
    XUtils::consoleOut(_msg_.str()); \
    return {}; \
}while(false)

#endif

#define CHECK_EMPTY(cond) CHECK_BASE(cond,TO_STRING(is empty!))
#define CHECK_ERR(cond) CHECK_BASE(cond,TO_STRING(is error!))

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
