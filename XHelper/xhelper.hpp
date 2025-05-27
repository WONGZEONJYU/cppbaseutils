#ifndef X_HELPER_HPP_
#define X_HELPER_HPP_

#include <utility>

#define XTD_VERSION "0.0.1"
#define XTD_NAMESPACE_BEGIN namespace xtd { inline namespace v1 {
#define XTD_NAMESPACE_END }}

#define X_DISABLE_COPY(Class) \
    Class(const Class &) = delete;\
    Class &operator=(const Class &) = delete;

#define X_DISABLE_COPY_MOVE(Class) \
    X_DISABLE_COPY(Class) \
    Class(Class &&) = delete; \
    Class &operator=(Class &&) = delete;

XTD_NAMESPACE_BEGIN

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
    uint32_t is_destroy:1{};
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
    uint32_t m_is_destroy_:1{};
};

XTD_NAMESPACE_END

#endif
