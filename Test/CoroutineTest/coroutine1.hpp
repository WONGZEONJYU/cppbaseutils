#ifndef XUTILS2_COROUTINE1_HPP
#define XUTILS2_COROUTINE1_HPP

#if 0

#include <coroutine>
#include <cstddef>
#include <cassert>
#include <XGlobal/xclasshelpermacros.hpp>

//XCoroutineGenerator
//XCoroutine

template<typename Promise>
struct Generator {

    using promise_type = Promise;
    using coroutine_handle = std::coroutine_handle<promise_type>;

private:
    coroutine_handle m_coroHandle_ {};
    bool m_isDestroy_ {};
    bool m_autoDestroy_ { true };

public:
    [[nodiscard]] constexpr auto & promiseRef() const
    { return m_coroHandle_.promise(); }

    constexpr void resume() const
    { if (m_coroHandle_) { m_coroHandle_.resume(); } }

    constexpr void operator()() const
    { resume(); }

    [[nodiscard]] bool done() const
    { return m_coroHandle_.done(); }

    explicit constexpr operator bool() const noexcept
    { return m_coroHandle_.operator bool(); }

    constexpr void isNeedAutoDestroy(bool const b = true) noexcept
    { m_autoDestroy_ = b; }

    [[nodiscard]] constexpr bool isAutoDestroy() const noexcept
    { return m_autoDestroy_; }

    constexpr void destroy()
        noexcept( noexcept(std::declval<coroutine_handle>().destroy()) )
    {
        if (m_coroHandle_ && !m_isDestroy_) {
            while (!m_coroHandle_.done()) { m_coroHandle_(); }
            m_isDestroy_ = true;
            m_coroHandle_.destroy();
        }
    }

    constexpr Generator() noexcept = default;

    constexpr Generator(coroutine_handle const h) noexcept
        : m_coroHandle_ { h } { assert(h); }

    X_DISABLE_COPY(Generator)

    Generator(Generator && o) noexcept
    { swap(o); }

    Generator & operator= (Generator && o) noexcept
    { swap(o); return *this; }

    constexpr void swap(Generator & o) noexcept {
        if (this != std::addressof(o)) {
            if (m_coroHandle_) { m_coroHandle_.destroy(); }
            m_coroHandle_ = o.m_coroHandle_;
            m_isDestroy_ = o.m_isDestroy_;
            m_autoDestroy_ = o.m_autoDestroy_;
            o.m_coroHandle_ = {};
            o.m_isDestroy_ = {};
            o.m_autoDestroy_ = true;
        }
    }

    virtual ~Generator() { if (isAutoDestroy()) { destroy(); } }

    friend bool operator==(Generator const & lhs, Generator const & rhs) noexcept
    { return lhs.m_coroHandle_ == rhs.m_coroHandle_; }

    friend std::strong_ordering operator<=>(Generator const & lhs, Generator const & rhs) noexcept
    { return lhs.m_autoDestroy_ <=> rhs.m_autoDestroy_; }

    static constexpr auto from_promise(promise_type * const p)
        noexcept(noexcept( coroutine_handle::from_promise(*p)))
    { return coroutine_handle::from_promise(*p); }

    static constexpr auto from_promise(promise_type & p)
        noexcept(noexcept( coroutine_handle::from_promise(p)))
    { return coroutine_handle::from_promise(p); }

    static constexpr auto from_address(void * const p) noexcept
    { return coroutine_handle::from_address(p); }

    [[nodiscard]] constexpr void * address() const noexcept
    { return m_coroHandle_.address(); }

    explicit operator std::coroutine_handle<> () const noexcept
    { return static_cast< std::coroutine_handle<> > ( m_coroHandle_ ); }

    friend struct std::hash<Generator>;
};

template<typename Promise>
constexpr void swap(Generator<Promise> & lhs,Generator<Promise> & rhs) noexcept
{ lhs.swap(rhs); }

template <typename Promise>
struct std::hash<Generator<Promise>> {

    constexpr size_t operator()(Generator<Promise> const & coro) const noexcept {

        std::size_t const h[] {
            std::hash<void*>{}(coro.m_coroHandle_.address())
            ,std::hash<bool>{}(coro.m_isDestroy_)
            , std::hash<bool>{}(coro.m_autoDestroy_)
        };

        static auto const hash_combine { [](std::size_t & seed, std::size_t const value) noexcept
        { seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2); } };

        std::size_t seed{};
        for (auto && item : h) { hash_combine(seed, item); }
        return seed;
    }
};

#endif

#endif
