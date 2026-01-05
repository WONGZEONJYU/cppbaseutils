#ifndef XUTILS2_X_COROUTINE_HPP
#define XUTILS2_X_COROUTINE_HPP 1

#include <coroutine>
#include <cassert>
#include <XGlobal/xclasshelpermacros.hpp>
#include <XHelper/xversion.hpp>
#include <XAtomic/xatomic.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template<typename PromiseType>
class XCoroutineAbstract {

public:
    using promise_type = PromiseType;
    using coroutine_handle = std::coroutine_handle<promise_type>;

private:
    #undef CHECK_NOEXCEPT_
    #define CHECK_NOEXCEPT_(Class,fn,...) \
    noexcept( noexcept( std::declval<Class>().fn( __VA_OPT__(, ) __VA_ARGS__ ) ) )

    #undef NOEXCEPT_
    #define NOEXCEPT_(fn) CHECK_NOEXCEPT_(coroutine_handle,fn)

    coroutine_handle m_coroHandle_ {};
    bool m_isDestroy_ {}
        ,m_autoDestroy_ { true };

public:
    static constexpr auto from_promise(promise_type * const p) NOEXCEPT_(from_promise(*p))
    { return coroutine_handle::from_promise(*p); }

    static constexpr auto from_promise(promise_type & p) NOEXCEPT_(from_promise(p))
    { return coroutine_handle::from_promise(p); }

    static constexpr auto from_address(void * const p) noexcept
    { return coroutine_handle::from_address(p); }

    [[nodiscard]] constexpr auto & promise() const NOEXCEPT_(promise)
    { assert(m_coroHandle_); return m_coroHandle_.promise(); }

    [[nodiscard]] bool done() const NOEXCEPT_(done)
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

    [[nodiscard]] constexpr auto address() const noexcept
    { assert(m_coroHandle_); return m_coroHandle_.address(); }

    constexpr operator std::coroutine_handle<> () const noexcept {
        assert(m_coroHandle_);
        return static_cast< std::coroutine_handle<> > ( m_coroHandle_ );
    }

    virtual ~XCoroutineAbstract()
    { if (m_autoDestroy_) { destroy(); } }

private:
    constexpr XCoroutineAbstract(coroutine_handle const & h)
        : m_coroHandle_{h} {}

    X_DISABLE_COPY(XCoroutineAbstract)

    template<typename > friend class XCoroutineGenerator;
    template<typename> friend struct XCoroutineGeneratorHash;
};

enum class CoroState : int {
    created,
    running,
    suspended,
    finished
};

template<typename PromiseType>
class XCoroutineGenerator : public XCoroutineAbstract<PromiseType> {

    X_DISABLE_COPY(XCoroutineGenerator)
    using Base = XCoroutineAbstract<PromiseType>;
    XAtomicInt m_currentStatus { static_cast<int>(CoroState::created) };

public:
    using promise_type = Base::promise_type;
    using coroutine_handle = Base::coroutine_handle;

    constexpr XCoroutineGenerator(coroutine_handle const h = {})
        :Base {h}
    {

    }

    constexpr void resume() const NOEXCEPT_(resume)
    { if (this->m_coroHandle_) { this->m_coroHandle_.resume(); } }

    constexpr void operator()() const NOEXCEPT_(resume)
    { resume(); }

    XCoroutineGenerator(XCoroutineGenerator && o) noexcept : Base{}
    { swap(o); }

    XCoroutineGenerator & operator=(XCoroutineGenerator && o) noexcept
    { swap(o); return *this; }

    friend bool operator==(XCoroutineGenerator const & lhs, XCoroutineGenerator const & rhs) noexcept
    { return lhs.m_coroHandle_ == rhs.m_coroHandle_; }

    friend std::strong_ordering operator<=>(XCoroutineGenerator const & lhs, XCoroutineGenerator const & rhs) noexcept
    { return lhs.m_autoDestroy_ <=> rhs.m_autoDestroy_; }

    constexpr void swap(XCoroutineGenerator & o) noexcept {
        if (this == std::addressof(o)) { return; }
        if (this->m_coroHandle_) { this->m_coroHandle_.destroy(); }
        this->m_coroHandle_ = o.m_coroHandle_;
        this->m_isDestroy_ = o.m_isDestroy_;
        this->m_autoDestroy_ = o.m_autoDestroy_;
        o.m_coroHandle_ = {};
        o.m_isDestroy_ = {};
        o.m_autoDestroy_ = true;
    }

    //friend struct std::hash<XCoroutineGenerator>;
    template<typename> friend struct XCoroutineGeneratorHash;
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
