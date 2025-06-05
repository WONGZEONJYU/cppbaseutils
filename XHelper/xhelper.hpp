#ifndef X_HELPER_HPP_
#define X_HELPER_HPP_

#include <string_view>
#include <utility>

#define XTD_VERSION "0.0.1"
#define XTD_NAMESPACE_BEGIN namespace xtd {
#define XTD_NAMESPACE_END }

#define XTD_INLINE_NAMESPACE_BEGIN(name) inline namespace name {
#define XTD_INLINE_NAMESPACE_END XTD_NAMESPACE_END

#define X_DISABLE_COPY(Class) \
    Class(const Class &) = delete;\
    Class &operator=(const Class &) = delete;

#define X_DISABLE_COPY_MOVE(Class) \
    X_DISABLE_COPY(Class) \
    Class(Class &&) = delete; \
    Class &operator=(Class &&) = delete;

template <typename T> inline T *xGetPtrHelper(T *ptr) noexcept { return ptr; }
template <typename Ptr> inline auto xGetPtrHelper(Ptr &ptr) noexcept -> decltype(ptr.get())
{ static_assert(noexcept(ptr.get()), "Smart d pointers for X_DECLARE_PRIVATE must have noexcept get()"); return ptr.get(); }

#define X_DECLARE_PRIVATE_D(D_ptr, Class) \
    inline Class* d_func() noexcept \
    { return reinterpret_cast<Class *>(xGetPtrHelper(D_ptr)); } \
    inline const Class* d_func() const noexcept \
    { return reinterpret_cast<const Class *>(xGetPtrHelper(D_ptr)); } \
    friend class Class;

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
public:
    constexpr inline explicit Destroyer(F &&f):
    fn(std::move(f)){}

    constexpr inline void destroy() {
        if (!is_destroy) {
            is_destroy = true;
            fn();
        }
    }

    constexpr inline ~Destroyer() {
        destroy();
    }

private:
    F fn;
    unsigned int is_destroy:1{};
};

template<typename F2>
class [[maybe_unused]] XRAII final {
    X_DISABLE_COPY_MOVE(XRAII)
public:

    [[maybe_unused]] constexpr inline explicit XRAII(auto &&f1,F2 &&f2):
    m_f2(std::move(f2)){
        f1();
    }

    constexpr inline void destroy(){
        if (!m_is_destroy_){
            m_is_destroy_ = true;
            m_f2();
        }
    }

    constexpr inline ~XRAII() {
        destroy();
    }

private:
    F2 m_f2{};
    unsigned int m_is_destroy_:1{};
};

void x_assert(const char *expr, const char *file,const int &line) noexcept;
void x_assert(const std::string &expr, const std::string &file,const int &line) noexcept;
void x_assert(const std::string_view &expr, const std::string_view &file,const int &line) noexcept;

void x_assert_what(const char *where, const char *what,
    const char *file,const int &line) noexcept;
void x_assert_what(const std::string &where, const std::string &what,
    const std::string &file,const int &line) noexcept;
void x_assert_what(const std::string_view &, const std::string_view &what,
    const std::string_view &file,const int &line) noexcept;

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
