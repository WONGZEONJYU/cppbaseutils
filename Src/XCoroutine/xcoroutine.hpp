#ifndef XUTILS2_X_COROUTINE_HPP
#define XUTILS2_X_COROUTINE_HPP 1

#include <coroutine>
#include <cassert>
#include <XGlobal/xclasshelpermacros.hpp>
#include <XHelper/xversion.hpp>
#include <XAtomic/xatomic.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

enum class CoroState : int {
    created,
    running,
    suspended,
    finished
};

struct XPromiseAbstract;

template<typename PromiseType>
struct XCoroutineGenerator {

    using promise_type = PromiseType;
    using coroutine_handle = std::coroutine_handle<promise_type>;

    static_assert(std::is_base_of_v<XPromiseAbstract,promise_type>
        ,"promise_type must inherit XPromiseAbstract!");

private:
    coroutine_handle m_coroHandle_ {};
    bool m_isDestroy_ {}
        ,m_autoDestroy_ { true };

#undef CHECK_NOEXCEPT_
#define CHECK_NOEXCEPT_(Class,fn,...) \
noexcept( noexcept( std::declval<Class>().fn( __VA_OPT__(, ) __VA_ARGS__ ) ) )

#undef NOEXCEPT_
#define NOEXCEPT_(fn) CHECK_NOEXCEPT_(coroutine_handle,fn)

public:
    static constexpr auto from_promise(promise_type * const p) NOEXCEPT_(from_promise(*p))
    { return coroutine_handle::from_promise(*p); }

    static constexpr auto from_promise(promise_type & p) NOEXCEPT_(from_promise(p))
    { return coroutine_handle::from_promise(p); }

    [[nodiscard]] constexpr auto & promise() const NOEXCEPT_(promise)
    { assert(m_coroHandle_); return m_coroHandle_.promise(); }

    [[nodiscard]] constexpr bool done() const NOEXCEPT_(done)
    { assert(m_coroHandle_); return m_coroHandle_.done(); }

    explicit constexpr operator bool() const noexcept
    { assert(m_coroHandle_); return m_coroHandle_.operator bool(); }

    constexpr void setAutoDestroy(bool const b = true) noexcept
    { m_autoDestroy_ = b; }

    [[nodiscard]] constexpr bool isAutoDestroy() const noexcept
    { return m_autoDestroy_; }

    constexpr void destroy() NOEXCEPT_(destroy) {
        if (m_coroHandle_ && !m_isDestroy_) {
            m_isDestroy_ = true;
            m_coroHandle_.destroy();
        }
    }

    virtual ~XCoroutineGenerator()
    { if (m_autoDestroy_) { destroy(); } }

    [[nodiscard]] constexpr bool tryResume() const {
        if (!operator bool() || done()) { return {} ; }
        m_coroHandle_.resume();
        return true;
    }

    constexpr bool operator()() const
    { return tryResume(); }

    constexpr explicit(false) XCoroutineGenerator(coroutine_handle const h = {})
        : m_coroHandle_{h} {}

    XCoroutineGenerator(XCoroutineGenerator && o) noexcept
    { swap(o); }

    XCoroutineGenerator & operator=(XCoroutineGenerator && o) noexcept
    { swap(o); return *this; }

    constexpr void swap(XCoroutineGenerator & o) noexcept {
        if (this == std::addressof(o)) { return; }
        if (m_coroHandle_) { m_coroHandle_.destroy(); }
        std::swap(m_coroHandle_, o.m_coroHandle_);
        std::swap(m_isDestroy_ , o.m_isDestroy_);
        std::swap(m_autoDestroy_, o.m_autoDestroy_);
    }

#if 0
    constexpr explicit(false) operator std::coroutine_handle<> () const noexcept {
        assert(m_coroHandle_);
        return static_cast< std::coroutine_handle<> > ( m_coroHandle_ );
    }

    static constexpr auto from_address(void * const p) noexcept
    { return coroutine_handle::from_address(p); }

    constexpr auto address() const NOEXCEPT_(address())
    { return m_coroHandle_.address(); }

#endif

    friend bool operator==(XCoroutineGenerator const & lhs, XCoroutineGenerator const & rhs) noexcept = default;

    friend std::strong_ordering operator<=>(XCoroutineGenerator const & lhs, XCoroutineGenerator const & rhs) noexcept {
        return std::tie( lhs.m_coroHandle_, lhs.m_isDestroy_,lhs.m_autoDestroy_ )
            <=> std::tie( rhs.m_coroHandle_, rhs.m_isDestroy_,rhs.m_autoDestroy_ );
    }

    template<typename> friend struct XCoroutineGeneratorHash;

    X_DISABLE_COPY(XCoroutineGenerator)

#undef CHECK_NOEXCEPT_
#undef NOEXCEPT_
};

template<typename Promise>
using XGeneratorCoro = XCoroutineGenerator<Promise>;

template<typename Promise>
constexpr void swap(XCoroutineGenerator<Promise> & lhs,XCoroutineGenerator<Promise> & rhs) noexcept
{ lhs.swap(rhs); }

template<typename> struct XCoroutineGeneratorHash;

template<typename Promise>
struct XCoroutineGeneratorHash<XCoroutineGenerator<Promise>> {
    constexpr std::size_t operator()(XCoroutineGenerator<Promise> const & coro) const noexcept {
        using coroutineHandle = XCoroutineGenerator<Promise>::coroutineHandle;
        std::size_t const h[] {
            std::hash<coroutineHandle>{}(coro.m_coroHandle_)
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

struct XFinalAwaiter {
    virtual ~XFinalAwaiter() = default;
    [[nodiscard]] virtual bool await_ready() noexcept { return {}; }
    virtual void await_suspend(std::coroutine_handle<>) noexcept {}
    virtual void await_resume() noexcept {}
};

struct XPromiseAbstract {

private:
    mutable XAtomicInt m_status_ { static_cast<int>(CoroState::created) };

protected:
    constexpr XPromiseAbstract() noexcept = default;

    constexpr void onCreated() const noexcept
    { m_status_.storeRelease(static_cast<int>(CoroState::created)); };

    constexpr void onRunning() const noexcept
    { m_status_.storeRelease(static_cast<int>(CoroState::running)); }

    constexpr void onSuspended() const noexcept
    { m_status_.storeRelease(static_cast<int>(CoroState::suspended)); }

    constexpr void onFinished() const noexcept
    { m_status_.storeRelease(static_cast<int>(CoroState::finished)); }

    template<typename > friend struct XCoroutineGenerator;

public:
    virtual std::suspend_always final_suspend() noexcept
    { return {}; }

    virtual ~XPromiseAbstract() = default;
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#if 0
template <typename Promise>
struct std::hash<XUtils::XCoroutineGenerator<Promise>> {

    constexpr std::size_t operator()(XUtils::XCoroutineGenerator<Promise> const & coro) const noexcept {

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
