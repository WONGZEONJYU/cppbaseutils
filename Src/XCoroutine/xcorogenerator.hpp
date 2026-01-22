#ifndef XUTILS2_X_CORO_GENERATOR_HPP
#define XUTILS2_X_CORO_GENERATOR_HPP 1

#pragma once

#include <XGlobal/xversion.hpp>
#include <XGlobal/xclasshelpermacros.hpp>
#include <variant>
#include <exception>
#include <coroutine>
#include <iostream>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template<typename> struct XGenerator;

namespace detail {

    template<typename T>
    class XGeneratorPromise {
        using value_type = std::remove_reference_t<T>;
        const void * m_value_ { };
        std::exception_ptr m_exception_ { };

    public:
        constexpr XGenerator<T> get_return_object() { return { this }; }

        static constexpr auto initial_suspend() noexcept
        { return std::suspend_always {}; }

        constexpr auto final_suspend() noexcept
        { m_value_ = {}; return std::suspend_always {}; }

        void unhandled_exception() noexcept
        { m_exception_ = std::current_exception(); }

        constexpr auto yield_value(value_type & value) noexcept
        { m_value_ = std::addressof(value); return std::suspend_always {}; }

        constexpr auto yield_value(value_type && value) noexcept
        { m_value_ = std::addressof(value); return std::suspend_always {}; }

        static constexpr void return_void() noexcept {}

        [[nodiscard]] constexpr auto exception() const noexcept
        { return m_exception_; }

        constexpr value_type & value() const noexcept
        { return *const_cast<value_type *>(static_cast<const value_type *>(m_value_)); }

        [[nodiscard]] constexpr auto finished() const noexcept
        { return !m_value_; }

        void rethrowIfException() const
        { if (m_exception_) { std::rethrow_exception(m_exception_); } }

        template<typename U>
        constexpr auto await_transform(U &&) = delete;
    };

}

template<typename T>
class XGeneratorIterator {
    using promise_type = detail::XGeneratorPromise<T>;
    std::coroutine_handle<promise_type> m_GeneratorCoroutine_ { };

public:
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::remove_reference_t<T>;
    using reference = std::add_lvalue_reference_t<T>;
    using pointer = std::add_pointer_t<value_type>;

    constexpr XGeneratorIterator operator++() {
        if (!m_GeneratorCoroutine_) { return *this; }
        m_GeneratorCoroutine_.resume(); // generate next value
        if (auto && promise { m_GeneratorCoroutine_.promise() };promise.finished()) {
            m_GeneratorCoroutine_ = {};
            promise.rethrowIfException();
        }
        return *this;
    }

    constexpr reference operator *() const noexcept
    { return m_GeneratorCoroutine_.promise().value(); }

    constexpr pointer operator->() const noexcept
    { return std::addressof(**this); }

    friend constexpr bool operator==(XGeneratorIterator const & lhs,XGeneratorIterator const & rhs) noexcept = default;
    friend constexpr std::strong_ordering operator<=>(XGeneratorIterator const & lhs,XGeneratorIterator const & rhs) noexcept
    { return lhs.m_GeneratorCoroutine_ <=> rhs.m_GeneratorCoroutine_; }

private:
    template<typename > friend struct XGenerator;

    constexpr XGeneratorIterator() noexcept = default;

    explicit(false) constexpr XGeneratorIterator(std::coroutine_handle<promise_type> const h)
        : m_GeneratorCoroutine_ { h }
    {   }
};

template<typename T>
struct XGenerator {
    using promise_type = detail::XGeneratorPromise<T>;
    friend promise_type;
    using iterator = XGeneratorIterator<T>;

private:
    using coroutine_handle = std::coroutine_handle<promise_type>;
    coroutine_handle m_generatorCoroutine_ { };

public:
    constexpr XGenerator() noexcept = default;

    constexpr XGenerator(XGenerator && o) noexcept
        : m_generatorCoroutine_ { std::move(o.m_generatorCoroutine_) }
    { o.m_generatorCoroutine_ = {}; }

    constexpr XGenerator &operator=(XGenerator && o) noexcept
    { XGenerator {std::move(o) }.swap(*this); return *this; }

    ~XGenerator()
    { if (m_generatorCoroutine_)  { m_generatorCoroutine_.destroy(); } }

    constexpr void swap(XGenerator & o) noexcept
    { std::swap(m_generatorCoroutine_,o.m_generatorCoroutine_); }

    iterator begin() const {
        m_generatorCoroutine_.resume(); // generate first value
        if (m_generatorCoroutine_.promise().finished()) { // did not yield anything
            m_generatorCoroutine_.promise().rethrowIfException();
            return {};
        }
        return { m_generatorCoroutine_ } ;
    }

    static constexpr iterator end() noexcept
    { return {}; }

private:
    explicit(false) constexpr XGenerator(promise_type * const promise)
        : m_generatorCoroutine_ { coroutine_handle::from_promise(*promise) }
    {   }

    explicit(false) constexpr XGenerator(promise_type & promise)
        : m_generatorCoroutine_ { coroutine_handle::from_promise(promise) }
    {   }

    X_DISABLE_COPY(XGenerator)
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
