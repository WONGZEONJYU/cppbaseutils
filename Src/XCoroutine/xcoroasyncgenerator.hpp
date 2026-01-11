#ifndef XUTILS2_X_CORO_ASYNC_GENERATOR_HPP
#define XUTILS2_X_CORO_ASYNC_GENERATOR_HPP 1

#include <XGlobal/xclasshelpermacros.hpp>
#define X_COROUTINE_
#include <XCoroutine/private/mixns_p.hpp>
#undef X_COROUTINE_

#include <coroutine>
#include <iterator>
#include <exception>
#include <tuple>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template<typename> struct XAsyncGenerator;

namespace detail {

    template<typename> class XAsyncGeneratorIterator;
    class XAsyncGeneratorYieldOperation;
    class XAsyncGeneratorAdvanceOperation;

    class XAsyncGeneratorPromiseAbstract : public AwaitTransformMixin {
        std::exception_ptr m_exception_ {};
        std::coroutine_handle<> m_consumerCoroutine_ {};

    protected:
        void * m_currentValue_ {};

    public:
        constexpr XAsyncGeneratorPromiseAbstract() noexcept = default;

        X_DISABLE_COPY(XAsyncGeneratorPromiseAbstract)
        X_DEFAULT_MOVE(XAsyncGeneratorPromiseAbstract)

        virtual ~XAsyncGeneratorPromiseAbstract() = default;

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

    protected:
        [[nodiscard]] constexpr XAsyncGeneratorYieldOperation internal_yield_value() const noexcept;

    private:
        friend class XAsyncGeneratorYieldOperation;
        friend class XAsyncGeneratorAdvanceOperation;
        friend class XIteratorAwaitableAbstract;
    };

    class XAsyncGeneratorYieldOperation final {
        std::coroutine_handle<> m_consume_ {};
    public:
        explicit(false) constexpr XAsyncGeneratorYieldOperation(std::coroutine_handle<> const h) noexcept
            : m_consume_ { h }
        {}

        static constexpr bool await_ready() noexcept
        { return false; }

        [[nodiscard]] constexpr auto await_suspend(std::coroutine_handle<>) const noexcept
        { return m_consume_; }

        static constexpr void await_resume() noexcept {}
    };

    constexpr auto XAsyncGeneratorPromiseAbstract::final_suspend() noexcept{
        m_currentValue_ = {};
        return internal_yield_value();
    }

    constexpr XAsyncGeneratorYieldOperation XAsyncGeneratorPromiseAbstract::internal_yield_value() const noexcept {
        return XAsyncGeneratorYieldOperation { m_consumerCoroutine_ };
    }

    class XIteratorAwaitableAbstract {
    protected:
        XAsyncGeneratorPromiseAbstract * m_promise_ { };
        std::coroutine_handle<> m_producerCoroutine_ { };

    public:
        static constexpr bool await_ready() noexcept
        { return {}; }

        [[nodiscard]] constexpr auto await_suspend(std::coroutine_handle<> const h) const noexcept
        { m_promise_->m_consumerCoroutine_ = h; return m_producerCoroutine_; }

    protected:
        constexpr XIteratorAwaitableAbstract() = default;
        constexpr XIteratorAwaitableAbstract( XAsyncGeneratorPromiseAbstract & promise
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

        constexpr XAsyncGeneratorYieldOperation yield_value(value_type & value) noexcept {
            m_currentValue_ = const_cast<std::remove_const_t<value_type> *>(std::addressof(value));
            return internal_yield_value();
        }

        XAsyncGeneratorYieldOperation yield_value(value_type && value) noexcept
        { return yield_value(value); }

        constexpr T & value() const noexcept
        { return *const_cast<value_type *>(static_cast<const value_type *>(m_currentValue_)); }
    };

    template<typename T>
    class XAsyncGeneratorPromise<T &&> final : public XAsyncGeneratorPromiseAbstract {
    public:
        constexpr XAsyncGeneratorPromise() noexcept = default;

        constexpr XAsyncGenerator<T> get_return_object() noexcept;

        constexpr XAsyncGeneratorYieldOperation yield_value(T && value) noexcept {
            m_currentValue_ = std::addressof(value);
            return internal_yield_value();
        }

        constexpr T && value() const noexcept
        { return std::move(*static_cast<T *>(m_currentValue_)); }
    };

}

template<typename T>
class XAsyncGeneratorIterator final {
    using promise_type = detail::XAsyncGeneratorPromise<T>;
    using handle_type = std::coroutine_handle<promise_type>;
    handle_type m_coroutine_ {};

    class IncrementIteratorAwaitable final : public detail::XIteratorAwaitableAbstract {
        using Base = XIteratorAwaitableAbstract;
        XAsyncGeneratorIterator & m_iterator_ {};
    public:
        explicit(false) constexpr IncrementIteratorAwaitable(XAsyncGeneratorIterator & iterator) noexcept
            : Base { iterator.m_coroutine_.promise(), iterator.m_coroutine_ },
              m_iterator_ {iterator} {}

        XAsyncGeneratorIterator & await_resume() {
            if (m_promise_->finished()) {
                m_iterator_ = {};
                m_promise_->rethrow_if_unhandled_exception();
            }
            return m_iterator_;
        }
    };

public:
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::remove_reference_t<T>;
    using reference = std::add_lvalue_reference_t<T>;
    using pointer = std::add_pointer_t<value_type>;

    explicit(false) constexpr XAsyncGeneratorIterator(std::nullptr_t) noexcept {}
    explicit(false) constexpr XAsyncGeneratorIterator(handle_type coroutine) noexcept
        : m_coroutine_ {coroutine} { }

    constexpr auto operator++() noexcept
    { return IncrementIteratorAwaitable { *this }; }

    constexpr reference operator *() const noexcept
    { return m_coroutine_.promise().value(); }

    friend constexpr bool operator==(XAsyncGeneratorIterator const & lhs
        , XAsyncGeneratorIterator const & rhs) = default;

    friend constexpr std::strong_ordering operator<=>( XAsyncGeneratorIterator const & lhs
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
        : m_coroutine_ { coroutine_handle::from_promise(promise) }
    { }

    constexpr XAsyncGenerator(XAsyncGenerator && o) noexcept
    { swap(o); }

    XAsyncGenerator& operator=(XAsyncGenerator && o) noexcept
    { swap(o); return *this; }

    virtual ~XAsyncGenerator()
    { if (m_coroutine_) { m_coroutine_.destroy(); } }

    auto begin() noexcept {

        class BeginIteratorAwaitable final : public detail::XIteratorAwaitableAbstract {
            using Base = XIteratorAwaitableAbstract;
        public:
            constexpr BeginIteratorAwaitable() noexcept = default;

            explicit BeginIteratorAwaitable(std::coroutine_handle<promise_type> const h) noexcept
                : Base (h.promise(), h) {}

            [[nodiscard]] constexpr bool await_ready() const noexcept
            { return m_promise_; }

            auto await_resume() {
                if (!m_promise_) { return iterator { {} }; }
                if (m_promise_->finished()) {
                    m_promise_->rethrow_if_unhandled_exception();
                    return iterator{ {} };
                }

                return iterator { coroutine_handle::from_promise(*static_cast<promise_type *>(m_promise_)) };
            }
        };

        return m_coroutine_ ? BeginIteratorAwaitable { m_coroutine_} : BeginIteratorAwaitable { };
    }

    /// Returns an iterator representing the finished generator.
    static constexpr auto end() noexcept
    { return iterator{}; }

    constexpr void swap(XAsyncGenerator & other) noexcept
    { std::swap(m_coroutine_, other.m_coroutine_); }

    X_DISABLE_COPY(XAsyncGenerator)
};

template<typename T>
constexpr void swap(XAsyncGenerator<T> & arg1, XAsyncGenerator<T> & arg2) noexcept
{ arg1.swap(arg2); }

template<typename T>
constexpr XAsyncGenerator<T> detail::XAsyncGeneratorPromise<T>::get_return_object() noexcept
{ return { *this }; }

template<typename T>
constexpr XAsyncGenerator<T> detail::XAsyncGeneratorPromise<T&&>::get_return_object() noexcept
{ return { *this }; }

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
