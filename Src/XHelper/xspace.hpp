#ifndef XUTILS2_X_RAII_HPP
#define XUTILS2_X_RAII_HPP 1

#include <XGlobal/xversion.hpp>
#include <XGlobal/xclasshelpermacros.hpp>
#include <functional>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

#if 1

/**
 * 这里为什么这样绕一层呢,是防止编译器一些警告
 */
template<typename ...Args>
auto bind(Args && ...args) -> decltype(std::bind(std::forward<Args>(args)...))
{ return std::bind(std::forward<Args>(args)...); }

#else

template <typename ...Args>
auto bind(Args && ...args)
{ return [&args...]{ return std::invoke(std::forward<Args>(args)...); }; }

#endif

class XSpace;

class AutoDestroyer {

    std::function<void()> m_fn_ {};
    mutable uint32_t m_is_destroy:1;

public:
    /**
     * 如果需参数,请使用 XUtils::bind(...) 或 std::bind(...)
     * Destroyer d {  XUtils::bing([](int){},1)  };
     */
    template<typename Fn_>
    constexpr explicit AutoDestroyer(Fn_ && f)
        : m_fn_ { std::forward<Fn_>(f) }
        , m_is_destroy {}
    {}

    constexpr void destroy() const {
        if (!m_is_destroy) {
            m_is_destroy = true;
            m_fn_();
        }
    }

    virtual ~AutoDestroyer() { destroy(); }

    X_DISABLE_COPY_MOVE(AutoDestroyer)
};

class XSpace final : public AutoDestroyer {

public:
    /**
     * 如果需参数,请使用 XUtils::bind(...) 或 std::bind(...)
     * X_RAII r { XUtils::bind([](int){},1) , XUtils::bind([](char){},'f') };
     */
    template<typename Fn1,typename Fn2>
    explicit constexpr XSpace(Fn1 && fn1,Fn2 && fn2)
        : AutoDestroyer { std::forward<Fn2>(fn2) }
    { std::forward<Fn1>(fn1)(); }

    ~XSpace() override = default;

    X_DISABLE_COPY_MOVE(XSpace)
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
