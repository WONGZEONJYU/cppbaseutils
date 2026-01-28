#ifndef XUTILS2_WAIT_FOR_HPP
#define XUTILS2_WAIT_FOR_HPP 1

#pragma once

#include <XCoroutine/xcoroutinetask.hpp>
#include <optional>
#include <QEventLoop>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    template <typename Awaitable> requires TaskConvertible<Awaitable>
    auto toTask(Awaitable && future) -> XCoroTask< convertible_awaitable_return_type_t<Awaitable> >
    { co_return co_await std::forward<Awaitable>(future); }

    class WaitContext {
        QEventLoop m_loop_ {};
        bool m_coroutineFinished_ {};
        std::exception_ptr m_exception_ {};

        void quit() noexcept
        { m_coroutineFinished_ = true; m_loop_.quit(); }

        void wait() {
            if (!m_coroutineFinished_) { m_loop_.exec(); }
            if (m_exception_) { std::rethrow_exception(m_exception_); }
        }

        template<Awaitable Awaitable>
        friend XCoroTask<> runCoroutine(WaitContext & , Awaitable && );

        template<typename T, Awaitable Awaitable>
        friend XCoroTask<> runCoroutine(WaitContext & context, std::optional<T> & , Awaitable && );

        template<typename T, Awaitable Awaitable>
        friend constexpr T waitFor(Awaitable && );
    };

    template<Awaitable Awaitable>
    XCoroTask<> runCoroutine(WaitContext & context, Awaitable && awaitable) {
        try { co_await std::forward<Awaitable>(awaitable); }
        catch (...) { context.m_exception_ = std::current_exception(); }
        context.quit();
    }

    template<typename T, Awaitable Awaitable>
    XCoroTask<> runCoroutine(WaitContext & context, std::optional<T> & result, Awaitable && awaitable) {
        try { result.emplace(co_await std::forward<Awaitable>(awaitable)); }
        catch (...) { context.m_exception_ = std::current_exception(); }
        context.quit();
    }

    template<typename T, Awaitable Awaitable>
    constexpr T waitFor(Awaitable && awaitable) {
        WaitContext context {};
        if constexpr (std::is_void_v<T>) {
            runCoroutine(context,std::forward<Awaitable>(awaitable));
            context.wait();
            return;
        } else {
            std::optional<T> result {};
            runCoroutine(context,result,std::forward<Awaitable>(awaitable));
            context.wait();
            return *result;
        }
    }

}

template<typename T>
constexpr T waitFor(XCoroTask<T> & task)
{ return detail::waitFor<T>(std::forward<XCoroTask<T>>(task)); }

template<typename T>
constexpr T waitFor(XCoroTask<T> && task)
{ return detail::waitFor<T>(std::forward<XCoroTask<T>>(task)); }

template<Awaitable Awaitable>
constexpr auto waitFor(Awaitable && awaitable)
{ return detail::waitFor< detail::awaitable_return_type_t< Awaitable > >(std::forward<Awaitable>(awaitable)); }

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
