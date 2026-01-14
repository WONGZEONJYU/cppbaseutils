#ifndef XUTILS2_Q_CORO_SIGNAL_HPP
#define XUTILS2_Q_CORO_SIGNAL_HPP 1

#pragma once

#include <XCoroutine/xcoroutinetask.hpp>
#include <XCoroutine/xcoroasyncgenerator.hpp>
#include <XQtHelper/coro/impl/isqprivatesignal.hpp>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <cassert>
#include <optional>
#include <deque>
#include <tuple>
#include <type_traits>
#include <chrono>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

namespace concepts {
    //! Simplistic QObject concept.
    template<typename T>
    concept QObject = requires(T * obj) {
        requires std::is_base_of_v<::QObject, T>;
        requires std::is_same_v<decltype(T::staticMetaObject), const QMetaObject>;
    };
}

    template<concepts::QObject T, typename FuncPtr>
    class QCoroSignalAbstract {

    protected:
        QPointer<T> m_obj_ {};
        FuncPtr m_funcPtr_ {};
        QMetaObject::Connection m_conn_ {};
        std::unique_ptr<QTimer> m_timeoutTimer_ {};

    private:
        template<typename...> struct concat_tuple;

        template<typename Arg, typename...TupleTypes>
        struct concat_tuple<Arg, std::tuple<TupleTypes...>>
        { using type = std::tuple<TupleTypes..., Arg>; };

        template<typename... Args>
        using concat_tuple_t = concat_tuple<Args...>::type;

        template<typename...> struct filtered_tuple;

        template<typename... Args>
        using filtered_tuple_t = filtered_tuple<Args...>::type;

        template<typename Tuple, typename Arg, typename... Args>
        struct filtered_tuple<Tuple, Arg, Args...> {
            using type = std::conditional_t<
                is_QPrivateSignal_v<Arg>, filtered_tuple_t<Tuple, Args...>,
                filtered_tuple_t<concat_tuple_t<Arg, Tuple>, Args...>
            >;
        };

        template<typename Tuple>
        struct filtered_tuple<Tuple> { using type = Tuple; };

        template<typename > struct args_tuple;

        template<typename  R, typename ... Args>
        struct args_tuple<R(Args...)> { using type = std::tuple<std::remove_cvref_t<Args>...>; };

        template<typename  R, typename  Obj, typename ... Args>
        struct args_tuple<R (Obj::*)(Args...)>
        { using type = filtered_tuple_t<std::tuple<>, std::remove_cvref_t<Args>...>; };

        using result_tuple_t = args_tuple<std::remove_cvref_t<FuncPtr>>::type;

        template<typename> struct result_type_from_tuple;

        template<typename Arg>
        struct result_type_from_tuple<std::tuple<Arg>> { using type = Arg; };

        template<typename... Args>
        struct result_type_from_tuple<std::tuple<Args...>> { using type = std::tuple<Args...>; };

        template<typename... Args>
        using result_type_from_tuple_t = result_type_from_tuple<Args...>::type;

    public:
        Q_DISABLE_COPY(QCoroSignalAbstract);

        virtual ~QCoroSignalAbstract()
        { if (static_cast<bool>(m_conn_)) { QObject::disconnect(m_conn_); } }

    protected:
        /**!
         * The result_type is std::optional of
         *  * T if result_tuple is std::tuple<T>
         *  * result_tuple otherwise
         **/
        using result_type = std::optional<result_type_from_tuple_t<result_tuple_t>>;

        void handleTimeout(std::coroutine_handle<> const h) {
            if (!m_timeoutTimer_) { return; }
            auto lambda { [this,h]{ QObject::disconnect(m_conn_); h.resume(); } };
            m_timeoutTimer_->callOnTimeout(m_obj_.get(),std::move(lambda),Qt::DirectConnection);
            // force coro to be resumed on our thread, not mObj's thread
            m_timeoutTimer_->start();
        }

        using milliseconds = std::chrono::milliseconds;

        constexpr QCoroSignalAbstract() = default;
        explicit(false) constexpr QCoroSignalAbstract(T * const obj, FuncPtr && funcPtr, milliseconds const timeout)
            : m_obj_{ obj }, m_funcPtr_ { std::forward<FuncPtr>(funcPtr) }
        {
            if (timeout.count() > -1) {
                m_timeoutTimer_ = std::make_unique<QTimer>();
                m_timeoutTimer_->setInterval(timeout);
                m_timeoutTimer_->setSingleShot(true);
            }
        }

        template<typename... Args>
        struct select_last { using type = decltype( (std::type_identity<Args>{}, ...) )::type; };

        template<typename... Args> using select_last_t = select_last<Args...>::type;

        template<typename StoreResultCb, typename... Args> requires(sizeof...(Args) > 0)
        constexpr void storeResult(StoreResultCb && storeResult, Args &&...args) {
            using LastArg = select_last_t<Args...>;
            if constexpr (is_QPrivateSignal_v<LastArg>) {
                // Based on https://stackoverflow.com/a/77026174/4601437
                // Remove the last element (which is a QPrivateSignal) from the tuple
                auto reduced { []<typename Tuple,std::size_t... I>(Tuple && tuple,std::index_sequence<I...>) constexpr
                    { return std::make_tuple(std::get<I>(std::forward<Tuple>(tuple))...); }
                    (std::forward_as_tuple(std::forward<Args>(args)...),std::make_index_sequence<sizeof...(Args) - 1> {})
                };
                // Use the shortened tuple as arguments to mResult.emplace()
                std::apply(std::forward<StoreResultCb>(storeResult), std::move(reduced));
            } else {
                std::invoke(std::forward<StoreResultCb>(storeResult), std::forward<Args>(args)...);
            }
        }

        template<typename StoreResultCb>
        constexpr void storeResult(StoreResultCb && storeResult)
        { std::invoke(std::forward<StoreResultCb>(storeResult)); }
    };

    template<concepts::QObject T, typename FuncPtr>
    class QCoroSignal : public QCoroSignalAbstract<T, FuncPtr> {
        using Base = QCoroSignalAbstract<T, FuncPtr>;

        Base::result_type m_result_ {};
        std::unique_ptr<QObject> m_dummyReceiver_ {};
        std::coroutine_handle<> m_awaitingCoroutine_ {};

    public:
        using result_type = Base::result_type;
        using typename Base::milliseconds;

        explicit(false) constexpr QCoroSignal(T * const obj, FuncPtr && ptr, milliseconds const timeout)
            : Base { obj, std::forward<FuncPtr>(ptr), timeout }
            , m_dummyReceiver_ { std::make_unique<QObject>() } {}

        Q_DISABLE_COPY(QCoroSignal);

        void swap(QCoroSignal & other) noexcept {
            std::ranges::swap(this->m_obj_, other.m_obj_);
            std::ranges::swap(this->m_funcPtr_, other.m_funcPtr_);
            std::ranges::swap(this->m_conn_, other.m_conn_);
            std::ranges::swap(this->m_timeoutTimer_, other.m_timeoutTimer_);
            std::ranges::swap(m_result_,other.m_result_);
            std::ranges::swap (m_dummyReceiver_,other.m_dummyReceiver_);
            std::ranges::swap(m_awaitingCoroutine_,other.m_awaitingCoroutine_);
            if (static_cast<bool>(this->m_conn_)) { QObject::disconnect(this->m_conn_); setupConnection(); }
        }

        QCoroSignal(QCoroSignal && other) noexcept
        { swap(other); }

        QCoroSignal &operator=(QCoroSignal && other) noexcept
        { swap(other); return *this; }

        ~QCoroSignal() override = default;

        bool await_ready() const noexcept
        { return this->m_obj_.isNull(); }

        void await_suspend(std::coroutine_handle<> const awaitingCoroutine) noexcept {
            this->handleTimeout(awaitingCoroutine);
            m_awaitingCoroutine_ = awaitingCoroutine;
            setupConnection();
        }

        result_type await_resume()
        { return std::move(m_result_); }

    private:
        void setupConnection() {

            Q_ASSERT(!this->m_conn_);

            this->m_conn_ = QObject::connect(this->m_obj_, this->m_funcPtr_, this->m_dummyReceiver_.get(),

                [this]<typename ...AS1>(AS1 && ...as1) {

                    if (this->m_timeoutTimer_) { this->m_timeoutTimer_->stop(); }

                    QObject::disconnect(this->m_conn_);

                    this->storeResult([this]<typename AS2>(AS2 && ...as2){ m_result_.emplace(std::forward<AS2>(as2)...); }, std::forward<AS1>(as1)...);

                    if (m_awaitingCoroutine_) { m_awaitingCoroutine_.resume(); }

                },Qt::QueuedConnection
            );
        }
    };

    template<concepts::QObject T, typename FuncPtr>
    QCoroSignal(T *, FuncPtr &&, std::chrono::milliseconds) -> QCoroSignal<T, FuncPtr>;

    template<concepts::QObject T, typename FuncPtr>
    class QCoroSignalQueue : public QCoroSignalAbstract<T, FuncPtr> {
        using Base = QCoroSignalAbstract<T, FuncPtr>;
        std::deque<typename Base::result_type::value_type> m_queue_ {};
        QObject m_dummyReceiver_ {};
        std::coroutine_handle<> m_awaitingCoroutine_ {};

    public:
        using typename Base::result_type;
        using typename Base::milliseconds;

        explicit(false) constexpr QCoroSignalQueue(T * const obj, FuncPtr && ptr, milliseconds const timeout)
            : Base { obj, std::forward<FuncPtr>(ptr), timeout }
        { setupConnection(); }

        Q_DISABLE_COPY_MOVE(QCoroSignalQueue);

        ~QCoroSignalQueue() override = default;

        auto operator co_await() noexcept {
            class Awaiter {
                QCoroSignalQueue * m_queue_ {};
            public:
                explicit(false) constexpr Awaiter(QCoroSignalQueue & queue) noexcept
                    : m_queue_ { std::addressof(queue) } {}

                bool await_ready() const noexcept
                { return !m_queue_->isValid() || !m_queue_->empty(); }

                void await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept {
                    m_queue_->handleTimeout(awaitingCoroutine);
                    m_queue_->setAwaiter(awaitingCoroutine);
                }

                result_type await_resume() { return m_queue_->dequeue(); }
            };

            return Awaiter {*this};
        }

    private:
        bool isValid() const noexcept
        { return !this->m_obj_.isNull(); }

        bool empty() const noexcept
        { return m_queue_.empty(); }

        result_type dequeue() {
            if (m_queue_.empty()) { return std::nullopt; }
            auto result { std::move(m_queue_.front()) };
            m_queue_.pop_front();
            return result;
        }

        void setAwaiter(std::coroutine_handle<> const h)
        { m_awaitingCoroutine_ = h; }

        void setupConnection() {

            if (static_cast<bool>(this->m_conn_)) { return; }

            auto lambda {

                [this]<typename ...A1>(A1 && ...a1) {

                    if (this->m_timeoutTimer_) { this->m_timeoutTimer_->stop(); }

                    auto f { [this]<typename ...A2>(A2 && ...a2) { m_queue_.emplace_back(std::forward<A2>(a2)...); } };

                    this->storeResult(std::move(f), std::forward<A1>(a1)... );

                    if (m_awaitingCoroutine_) { m_awaitingCoroutine_.resume(); }

                }
            };

            this->m_conn_ = QObject::connect(this->m_obj_, this->m_funcPtr_
                , std::addressof(m_dummyReceiver_),std::move(lambda), Qt::QueuedConnection);
        }
    };

    template<concepts::QObject T, typename FuncPtr>
    QCoroSignalQueue(T *, FuncPtr &&, std::chrono::milliseconds) -> QCoroSignalQueue<T, FuncPtr>;

}

template<detail::concepts::QObject T, typename FuncPtr>
auto qCoro(T * const obj, FuncPtr && ptr, std::chrono::milliseconds const timeout)
    -> XCoroTask<typename detail::QCoroSignal<T, FuncPtr>::result_type>
{
    auto result { co_await detail::QCoroSignal(obj,std::forward<FuncPtr>(ptr), timeout) };
    co_return std::move(result);
}

template<detail::concepts::QObject T, typename FuncPtr>
auto qCoro(T * const obj, FuncPtr && ptr)
    -> XCoroTask< typename detail::QCoroSignal<T, FuncPtr>::result_type::value_type >
{
    auto result { co_await qCoro<T, FuncPtr>(obj, std::forward<FuncPtr>(ptr), std::chrono::milliseconds{-1}) };
    co_return std::move(*result);
}

template<detail::concepts::QObject T, typename FuncPtr>
auto qCoroSignalListener(T * const obj, FuncPtr && ptr,std::chrono::milliseconds const timeout = std::chrono::milliseconds{-1})
    -> XAsyncGenerator<typename detail::QCoroSignalQueue<T, FuncPtr>::result_type::value_type>
{

    using SignalQueue = detail::QCoroSignalQueue<T, FuncPtr>;

    // The actual generator is in a wrapper function, so that we can perform
    // some initialization (constructing signalQueue) in the qCoroSignalListener()
    // function before the generator gets initially suspended.

    constexpr auto innerGenerator { [](std::unique_ptr<SignalQueue> signalQueue)
        -> XAsyncGenerator<typename SignalQueue::result_type::value_type>
        {
            while (true) {
                auto result {co_await *signalQueue };
                if (!result.has_value()) { break; } // timeout
                co_yield std::move(*result);
            }
        }
    };

    return innerGenerator(std::make_unique<SignalQueue>(obj, std::forward<FuncPtr>(ptr), timeout));
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
