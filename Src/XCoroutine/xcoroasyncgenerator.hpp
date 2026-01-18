#ifndef XUTILS2_X_CORO_ASYNC_GENERATOR_HPP
#define XUTILS2_X_CORO_ASYNC_GENERATOR_HPP 1

#include <XGlobal/xclasshelpermacros.hpp>
#define X_COROUTINE_
#include <XCoroutine/private/mixns_p.hpp>
#undef X_COROUTINE_

#include <coroutine>
#include <iterator>
#include <exception>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template<typename> struct XAsyncGenerator;

namespace detail {

    template<typename> class XAsyncGeneratorIterator;
    class XAsyncGeneratorYieldOperation;
    class XAsyncGeneratorAdvanceOperation;

    class XAsyncGeneratorPromiseAbstract : public AwaitTransformMixin {

        friend class XAsyncGeneratorYieldOperation;
        friend class XAsyncGeneratorAdvanceOperation;
        friend class XIteratorAwaitableAbstract;

        std::exception_ptr m_exception_ {};
        std::coroutine_handle<> m_consumerCoroutine_ {};

    protected:
        void * m_currentValue_ {};

    public:
        static constexpr auto initial_suspend() noexcept
        { return std::suspend_always {}; }

        constexpr auto final_suspend() noexcept;

        void unhandled_exception() noexcept
        { m_exception_ = std::current_exception(); }

        static constexpr void return_void() noexcept {}

        [[nodiscard]] constexpr bool finished() const noexcept
        { return !m_currentValue_; }

        void rethrow_if_unhandled_exception() const
        { if (m_exception_) { std::rethrow_exception(m_exception_); } }

        virtual ~XAsyncGeneratorPromiseAbstract() = default;

        X_DISABLE_COPY(XAsyncGeneratorPromiseAbstract)
        X_DEFAULT_MOVE(XAsyncGeneratorPromiseAbstract)

    protected:
        [[nodiscard]] constexpr XAsyncGeneratorYieldOperation internal_yield_value() const noexcept;
        constexpr XAsyncGeneratorPromiseAbstract() noexcept = default;
    };

    class XAsyncGeneratorYieldOperation final {
        std::coroutine_handle<> m_consume_ {};
    public:
        explicit(false) constexpr XAsyncGeneratorYieldOperation(std::coroutine_handle<> const h) noexcept
            : m_consume_ { h } { }

        static constexpr bool await_ready() noexcept
        { return {}; }

        [[nodiscard]] constexpr auto await_suspend(std::coroutine_handle<>) const noexcept
        { return m_consume_; }

        static constexpr void await_resume() noexcept {}
    };

    constexpr auto XAsyncGeneratorPromiseAbstract::final_suspend() noexcept
    { m_currentValue_ = {}; return internal_yield_value(); }

    constexpr auto XAsyncGeneratorPromiseAbstract::internal_yield_value() const noexcept
        -> XAsyncGeneratorYieldOperation
    { return { m_consumerCoroutine_ }; }

    class XIteratorAwaitableAbstract {
    protected:
        XAsyncGeneratorPromiseAbstract * m_promise_ {};
        std::coroutine_handle<> m_producerCoroutine_ {};

    public:
        static constexpr bool await_ready() noexcept
        { return {}; }

        [[nodiscard]] constexpr auto await_suspend(std::coroutine_handle<> const h) const noexcept
        { m_promise_->m_consumerCoroutine_ = h; return m_producerCoroutine_; }

    protected:
        constexpr XIteratorAwaitableAbstract() = default;
        explicit(false) constexpr XIteratorAwaitableAbstract( XAsyncGeneratorPromiseAbstract & promise
                                            ,std::coroutine_handle<> const h) noexcept
            : m_promise_ { std::addressof(promise) } , m_producerCoroutine_ { h }
        { }
    };

    template<typename T>
    class XAsyncGeneratorPromise final : public XAsyncGeneratorPromiseAbstract {
        using value_type = std::remove_reference_t<T>;
    public:
        constexpr XAsyncGeneratorPromise() noexcept = default;

        constexpr XAsyncGenerator<T> get_return_object() noexcept;

        constexpr auto yield_value(value_type & value) noexcept
            -> XAsyncGeneratorYieldOperation
        {
            m_currentValue_ = const_cast<std::remove_const_t<value_type> *>(std::addressof(value));
            return internal_yield_value();
        }

        constexpr auto yield_value(value_type && value) noexcept
        { return yield_value(value); }

        constexpr T & value() const noexcept
        { return *const_cast<value_type *>(static_cast<const value_type *>(m_currentValue_)); }
    };

    template<typename T>
    class XAsyncGeneratorPromise<T &&> final : public XAsyncGeneratorPromiseAbstract {
    public:
        constexpr XAsyncGeneratorPromise() noexcept = default;

        constexpr XAsyncGenerator<T> get_return_object() noexcept;

        constexpr auto yield_value(T && value) noexcept
            -> XAsyncGeneratorYieldOperation
        {
            m_currentValue_ = std::addressof(std::forward<T>(value));
            return internal_yield_value();
        }

        constexpr T && value() const noexcept
        { return std::move(*static_cast<T*>(m_currentValue_)); }
    };

}

template<typename T>
class XAsyncGeneratorIterator final {
    using promise_type = detail::XAsyncGeneratorPromise<T>;
    using coroutine_handle = std::coroutine_handle<promise_type>;
    coroutine_handle m_coroutine_ {};

    class IncrementIteratorAwaitable final
        : public detail::XIteratorAwaitableAbstract
    {
        using Base = XIteratorAwaitableAbstract;
        XAsyncGeneratorIterator * m_iterator_ {};

    public:
        explicit(false) constexpr IncrementIteratorAwaitable(XAsyncGeneratorIterator & iterator) noexcept
            : Base { iterator.m_coroutine_.promise(), iterator.m_coroutine_ },
              m_iterator_ { std::addressof(iterator) } {}

        XAsyncGeneratorIterator & await_resume() {
            if (m_promise_->finished()) {
                *m_iterator_ = {};
                m_promise_->rethrow_if_unhandled_exception();
            }
            return *m_iterator_;
        }
    };

public:
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::remove_reference_t<T>;
    using reference = std::add_lvalue_reference_t<T>;
    using pointer = std::add_pointer_t<value_type>;

    constexpr XAsyncGeneratorIterator() noexcept = default;

    explicit(false) constexpr XAsyncGeneratorIterator(coroutine_handle const h) noexcept
        : m_coroutine_ { h } { }

    constexpr auto operator++() noexcept
    { return IncrementIteratorAwaitable { *this }; }

    constexpr reference operator *() const noexcept
    { return m_coroutine_.promise().value(); }

    constexpr pointer operator->() const noexcept
    { return std::addressof(**this); /* std::addressof(operator *()) */ }

    friend constexpr bool operator==(XAsyncGeneratorIterator const & lhs
        , XAsyncGeneratorIterator const & rhs) = default;

    friend constexpr std::strong_ordering operator<=>(XAsyncGeneratorIterator const & lhs
        ,XAsyncGeneratorIterator const & rhs) noexcept {
        return lhs.m_coroutine_ <=> rhs.m_coroutine_;
    }
};

template<typename T>
struct XAsyncGenerator {
    using promise_type = detail::XAsyncGeneratorPromise<T>;
    using iterator = XAsyncGeneratorIterator<T>;

private:
    using coroutine_handle = std::coroutine_handle<promise_type>;
    coroutine_handle m_coroutine_ { };

public:
    constexpr XAsyncGenerator() noexcept = default;

    explicit(false) constexpr XAsyncGenerator(promise_type & promise) noexcept
        : m_coroutine_ { coroutine_handle::from_promise(promise) } { }

    explicit(false) constexpr XAsyncGenerator(promise_type * const promise) noexcept
        : XAsyncGenerator { *promise } { }

    constexpr XAsyncGenerator(XAsyncGenerator && o) noexcept
    { swap(o); }

    XAsyncGenerator& operator=(XAsyncGenerator && o) noexcept
    { swap(o); return *this; }

    virtual ~XAsyncGenerator()
    { if (m_coroutine_) { m_coroutine_.destroy(); } }

    constexpr auto begin() noexcept {

        class BeginIteratorAwaitable final
            : public detail::XIteratorAwaitableAbstract
        {
            using Base = XIteratorAwaitableAbstract;
        public:
            constexpr BeginIteratorAwaitable() noexcept = default;

            explicit(false) constexpr BeginIteratorAwaitable(coroutine_handle const h) noexcept
                : Base { h.promise(), h } {}

            [[nodiscard]] constexpr bool await_ready() const noexcept
            { return m_promise_; }

            constexpr iterator await_resume() const {
                if (!m_promise_) { return { }; }
                if (m_promise_->finished()) { m_promise_->rethrow_if_unhandled_exception(); return { }; }
                return { coroutine_handle::from_promise(*static_cast<promise_type *>(m_promise_)) };
            }
        };

        return m_coroutine_ ? BeginIteratorAwaitable { m_coroutine_ } : BeginIteratorAwaitable { };
    }

    static constexpr auto end() noexcept
    { return iterator {}; }

    constexpr void swap(XAsyncGenerator & other) noexcept
    { std::swap(m_coroutine_, other.m_coroutine_); }

    X_DISABLE_COPY(XAsyncGenerator)
};

template<typename T>
constexpr void swap(XAsyncGenerator<T> & arg1, XAsyncGenerator<T> & arg2) noexcept
{ arg1.swap(arg2); }

template<typename T>
constexpr XAsyncGenerator<T> detail::XAsyncGeneratorPromise<T>::get_return_object() noexcept
{ return { this }; }

template<typename T>
constexpr XAsyncGenerator<T> detail::XAsyncGeneratorPromise<T&&>::get_return_object() noexcept
{ return { this }; }

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#define X_CORO_FOREACH(var, generator) \
    if (auto && _container_ { (generator) }; false) {} else \
    for (auto _begin_ { co_await _container_.begin() }, _end_ { _container_.end() }; _begin_ != _end_; co_await ++_begin_) \
    if (var = *_begin_; false) {} else // NOLINT(bugprone-macro-parentheses)

#endif
