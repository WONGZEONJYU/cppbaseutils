#ifndef XUTILS2_X_RAII_HPP
#define XUTILS2_X_RAII_HPP 1

#include <XHelper/xversion.hpp>
#include <XGlobal/xclasshelpermacros.hpp>
#include <functional>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

#if 1

/**
 * 这里为什么这样绕一层呢,是防止编译器一些警告
 */
template<typename ...Args>
auto bind(Args && ...args) -> decltype(std::bind(std::forward<decltype(args)>(args)...))
{ return std::bind(std::forward<decltype(args)>(args)...); }

#else

template <typename ...Args>
auto bind(Args && ...args)
{ return [&args...]{ return std::invoke(std::forward<decltype(args)>(args)...); }; }

#endif

class X_RAII;

class Destroyer {

    std::function<void()> m_fn_ {};
    mutable uint32_t m_is_destroy:1;

public:
    /**
     * 如果需参数,请使用 XUtils::bind(...) 或 std::bind(...)
     * Destroyer d {  XUtils::bing([](int){},1)  };
     */
    template<typename Fn_>
    constexpr explicit Destroyer(Fn_ && f)
        : m_fn_ { std::forward<decltype(f)>(f) }
        , m_is_destroy {}
    {}

    constexpr void destroy() const {
        if (!m_is_destroy) {
            m_is_destroy = true;
            m_fn_();
        }
    }

    constexpr virtual ~Destroyer() { destroy(); }

    X_DISABLE_COPY_MOVE(Destroyer)
};

class X_RAII final : public Destroyer {

public:
    /**
     * 如果需参数,请使用 XUtils::bind(...) 或 std::bind(...)
     * X_RAII r { XUtils::bind([](int){},1) , XUtils::bind([](char){},'f') };
     */
    template<typename Fn1,typename Fn2>
    constexpr explicit X_RAII(Fn1 && fn1,Fn2 && fn2)
        : Destroyer { std::forward<decltype(fn2)>(fn2) }
    { std::forward<decltype(fn1)>(fn1)(); }

    ~X_RAII() override = default;

    X_DISABLE_COPY_MOVE(X_RAII)
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
