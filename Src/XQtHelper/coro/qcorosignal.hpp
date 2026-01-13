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
        QMetaObject::Connection m_conn_ { };
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

        using result_tuple = args_tuple<std::remove_cvref_t<FuncPtr>>::type;

        template<typename> struct result_type_from_tuple;

        template<typename Arg>
        struct result_type_from_tuple<std::tuple<Arg>> { using type = Arg; };

        template<typename... Args>
        struct result_type_from_tuple<std::tuple<Args...>> { using type = std::tuple<Args...>; };

        template<typename... Args>
        using result_type_from_tuple_t = result_type_from_tuple<Args...>::type;

    public:
        /**!
         * The result_type is std::optional of
         *  * T if result_tuple is std::tuple<T>
         *  * result_tuple otherwise
         **/
        using result_type = std::optional<result_type_from_tuple_t<result_tuple>>;

        Q_DISABLE_COPY(QCoroSignalAbstract)
        X_DEFAULT_MOVE(QCoroSignalAbstract)

        virtual ~QCoroSignalAbstract()
        { if (static_cast<bool>(m_conn_)) { QObject::disconnect(m_conn_); } }

        void handleTimeout(std::coroutine_handle<> const h) {
            if (m_timeoutTimer_) {
                m_timeoutTimer_->callOnTimeout([this,h]{
                    QObject::disconnect(m_conn_);
                    h.resume();
                },Qt::DirectConnection); // force coro to be resumed on our thread, not mObj's thread
                m_timeoutTimer_->start();
            }
        }

    protected:
        using milliseconds = std::chrono::milliseconds;
        explicit(false) constexpr QCoroSignalAbstract(T * const obj, FuncPtr && funcPtr, milliseconds const timeout)
            : m_obj_{ obj }, m_funcPtr_(std::forward<FuncPtr>(funcPtr))
        {
            if (timeout.count() > -1) {
                m_timeoutTimer_ = std::make_unique<QTimer>();
                m_timeoutTimer_->setInterval(timeout);
                m_timeoutTimer_->setSingleShot(true);
            }
        }

        template<typename... Args>
        struct select_last
        { using type = decltype( (std::type_identity<Args>{}, ...) )::type; };

        template<typename... Args>
        using select_last_t = select_last<Args...>::type;

        template<typename StoreResultCb, typename... Args> requires(sizeof...(Args) > 0)
        constexpr void storeResult(StoreResultCb && storeResult, Args &&...args) {
            using LastArg = select_last_t<Args...>;
            if constexpr (is_QPrivateSignal_v<LastArg>) {
                // Based on https://stackoverflow.com/a/77026174/4601437
                // Remove the last element (which is a QPrivateSignal) from the tuple
                auto reduced { []<typename Args_,std::size_t... I>(Args_ && all,std::index_sequence<I...>) constexpr
                    { return std::make_tuple(std::get<I>(all)...); }
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

}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
