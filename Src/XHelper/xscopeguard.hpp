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

template<typename Fn>
class AutoDestroyer {

    Fn m_fn_{};
    mutable bool m_is_destroy{};

public:
    /**
     * 如果需参数,请使用 XUtils::bind(...) 或 std::bind(...)
     * Destroyer d {  XUtils::bing([](int){},1)  };
     */
    constexpr explicit AutoDestroyer(Fn && f) noexcept
        : m_fn_ { std::move(f) }
    {   }

    constexpr AutoDestroyer(AutoDestroyer && other) noexcept
        :m_fn_ { std::move(other.m_fn_) }
        ,m_is_destroy { std::exchange(other.m_is_destroy,{}) }
    {   }

    constexpr void destroy() const noexcept {
        if (m_is_destroy) { return; }
        m_is_destroy = true;
        m_fn_();
    }

    constexpr void dismiss() const noexcept
    { m_is_destroy = true; }

    virtual ~AutoDestroyer() noexcept
    { destroy(); }

    X_DISABLE_COPY(AutoDestroyer)
};

template <typename F> AutoDestroyer(F(&)()) -> AutoDestroyer<F(*)()>;

template<typename Release>
class XScopeGuard final : public AutoDestroyer<Release> {
    using Base = AutoDestroyer<Release>;
public:
    /**
     * 如果需参数,请使用 XUtils::bind(...) 或 std::bind(...)
     * XScopeGuard guard { XUtils::bind([](int){},1) , XUtils::bind([](char){},'f') };
     */
    template<typename Fn>
    explicit constexpr XScopeGuard(Fn && fn,Release && release) noexcept
        : Base { std::move(release) }
    { std::forward<Fn>(fn)(); }

    ~XScopeGuard() override = default;

    constexpr XScopeGuard(XScopeGuard &&) noexcept = default;

    X_DISABLE_COPY(XScopeGuard)
};

template <typename Fn, typename F>
XScopeGuard(Fn&&, F(&)()) -> XScopeGuard<F(*)()>;

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
