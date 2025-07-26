#ifndef X_OVERLOAD_HPP
#define X_OVERLOAD_HPP 1

template <typename... Args>
class XNonConstOverload {
public:
    template <typename R, typename T>
    inline constexpr auto operator()(R (T::*ptr)(Args...)) const noexcept -> decltype(ptr)
    { return ptr; }

    template <typename R, typename T>
    inline static constexpr auto of(R (T::*ptr)(Args...)) noexcept -> decltype(ptr)
    { return ptr; }
};

template <typename... Args>
class XConstOverload {
public:
    template <typename R, typename T>
    constexpr auto operator()(R (T::*ptr)(Args...) const) const noexcept -> decltype(ptr)
    { return ptr; }

    template <typename R, typename T>
    static constexpr auto of(R (T::*ptr)(Args...) const) noexcept -> decltype(ptr)
    { return ptr; }
};

template <typename... Args>
class XOverload : public XConstOverload<Args...>, public XNonConstOverload<Args...> {
    using XConstOverload<Args...>::of;
    using XConstOverload<Args...>::operator();
    using XNonConstOverload<Args...>::of;
    using XNonConstOverload<Args...>::operator();

    template <typename R>
    inline constexpr auto operator()(R (*ptr)(Args...)) const noexcept -> decltype(ptr)
    { return ptr; }

    template <typename R>
    inline static constexpr auto of(R (*ptr)(Args...)) noexcept -> decltype(ptr)
    { return ptr; }
};

template <typename... Args> [[maybe_unused]] constexpr inline XOverload<Args...> xOverload {};
template <typename... Args> [[maybe_unused]] constexpr inline XConstOverload<Args...> xConstOverload {};
template <typename... Args> [[maybe_unused]] constexpr inline XNonConstOverload<Args...> xNonConstOverload {};

#endif
