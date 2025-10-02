#ifndef X_HELPER_HPP_
#define X_HELPER_HPP_

#include <XHelper/xversion.hpp>
#include <XHelper/xtypetraits.hpp>
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

template<typename T,typename > class XTwoPhaseConstruction;
template<typename T,typename > class XSingleton;

class X_CLASS_EXPORT XUtilsLibErrorLog final {
    constexpr XUtilsLibErrorLog() = default;
    static void log(std::string_view const & );
    template<typename ,typename > friend class XSingleton;
    template<typename ,typename > friend class XTwoPhaseConstruction;
};

template <typename T> constexpr T *xGetPtrHelper(T *ptr) noexcept { return ptr; }
template <typename Ptr> constexpr auto xGetPtrHelper(Ptr &ptr) noexcept -> decltype(ptr.get())
{ static_assert(noexcept(ptr.get()), "Smart d pointers for X_DECLARE_PRIVATE must have noexcept get()"); return ptr.get(); }
template <typename Ptr> constexpr auto xGetPtrHelper(Ptr const &ptr) noexcept -> decltype(ptr.get())
{ static_assert(noexcept(ptr.get()), "Smart d pointers for X_DECLARE_PRIVATE must have noexcept get()"); return ptr.get(); }

template<typename F>
class Destroyer {
    X_DISABLE_COPY_MOVE(Destroyer)
    mutable F m_fn_{};
    mutable uint32_t m_is_destroy:1;

public:
    constexpr explicit Destroyer(F && f):
    m_fn_(std::move(f)),m_is_destroy{}{}

    constexpr void destroy() const {
        if (!m_is_destroy) {
            m_is_destroy = true;
            m_fn_();
        }
    }
    constexpr virtual ~Destroyer() { destroy();}
};

template<typename F2>
class X_RAII final : public Destroyer<F2> {
    using Base = Destroyer<F2>;
    X_DISABLE_COPY_MOVE(X_RAII)

public:
    template<typename F1>
    constexpr explicit X_RAII(F1 && f1,F2 && f2)
    : Base { std::forward<F2>(f2) }
    { f1(); }

    ~X_RAII() override = default;
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

enum class ConnectionType {
    AutoConnection,
    DirectConnection,
    QueuedConnection,
    BlockingQueuedConnection,
    UniqueConnection
};

enum class NonConst{};
enum class Const{};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
