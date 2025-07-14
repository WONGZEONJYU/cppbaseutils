#ifndef X_HELPER_HPP_
#define X_HELPER_HPP_

#include <string>
#include <XHelper/xversion.hpp>
#include <string_view>
#include <utility>
#include <memory>

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

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template<typename F>
class [[maybe_unused]] Destroyer final {
    X_DISABLE_COPY_MOVE(Destroyer)
    mutable F m_fn_;
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
 * 错误输出
 * @param expr
 * @param file
 * @param line
 */
void x_assert(const char *expr, const char *file,const int &line) noexcept;
void x_assert(const std::string &expr, const std::string &file,const int &line) noexcept;
void x_assert(const std::string_view &expr, const std::string_view &file,const int &line) noexcept;

/**
 * 错误输出
 * @param where
 * @param what
 * @param file
 * @param line
 */
void x_assert_what(const char *where, const char *what,
                   const char *file,const int &line) noexcept;
void x_assert_what(const std::string &where, const std::string &what,
    const std::string &file,const int &line) noexcept;
void x_assert_what(const std::string_view &, const std::string_view &what,
    const std::string_view &file,const int &line) noexcept;

/**
 * 创建std::unique_ptr<T>,不抛异常
 * @tparam T
 * @tparam Args
 * @param args
 * @return
 */
template<typename T, typename ... Args>
[[maybe_unused]] [[nodiscard]] inline std::unique_ptr<T> make_Unique(Args && ...args) noexcept {
    try{
        return std::make_unique<T>(std::forward<Args>(args)...);
    }catch (const std::exception &){
        return {};
    }
}

/**
 * 创建std::shared_ptr<T>
 * @tparam T
 * @tparam Args
 * @param args
 * @return
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
 *
 * @tparam Tuple
 * @tparam Pred
 * @param tuple_
 * @param pred_
 */
template<typename Tuple, typename Pred>
[[maybe_unused]] inline void for_each_tuple(Tuple && tuple_, Pred && pred_) {
    (void)std::apply([&pred_]<typename... Args>(Args &&... args) {
        (std::forward<Pred>(pred_)(std::forward<Args>(args)), ...);
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
inline auto Left_Tuple(const Tuple & tuple_)
->decltype(TuplePrivate::Left_Tuple_n_<N>(tuple_)) {
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
inline auto Last_Tuple(const Tuple & tuple_)
    ->decltype(TuplePrivate::Last_Tuple_n_<N>(tuple_)) {
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
inline auto SkipFront_Tuple(const Tuple &tuple_)
    ->decltype(TuplePrivate::skip_front_n_<N>(tuple_)) {
    return TuplePrivate::skip_front_n_<N>(tuple_);
}

[[maybe_unused]] std::string toLower(std::string &);

[[maybe_unused]] std::string toLower(std::string &&);

[[maybe_unused]] std::string toLower(const std::string &);

[[maybe_unused]] std::string toLower(std::string_view &);

[[maybe_unused]] std::string toLower(std::string_view &&);

[[maybe_unused]] std::string toLower(const std::string_view &);

[[maybe_unused]] std::string toUpper(std::string &);

[[maybe_unused]] std::string toUpper(std::string &&);

[[maybe_unused]] std::string toUpper(const std::string &);

[[maybe_unused]] std::string toUpper(std::string_view &);

[[maybe_unused]] std::string toUpper(std::string_view &&);

[[maybe_unused]] std::string toUpper(const std::string_view &);

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
