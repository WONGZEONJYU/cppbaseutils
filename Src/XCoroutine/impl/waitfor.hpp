#ifndef XUTILS2_WAIT_FOR_HPP
#define XUTILS2_WAIT_FOR_HPP 1

#pragma once

#include <XHelper/xqt_detection.hpp>
#include <optional>
#include <thread>

#ifdef HAS_QT
#include <QEventLoop>
#else
/**
 * 此处只是防止编译出错,本质是没有任何意义的
 */
struct QEventLoop {
    constexpr QEventLoop() = default;
    void quit() noexcept { m_is_quit_.storeRelaxed(true); }
    int exec() const noexcept
    { while (!m_is_quit_.loadRelaxed()) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); } return 0;}
private:
    XUtils::XAtomicBool m_is_quit_ {};
};
#endif

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {

    template <typename Awaitable> requires TaskConvertible<Awaitable>
    auto toTask(Awaitable && future)
        -> XCoroTask< convertible_awaitable_return_type_t<Awaitable> >
    { co_return co_await std::forward<Awaitable>(future); }

    struct WaitContext {
        QEventLoop m_loop {};
        bool m_coroutineFinished {};
        std::exception_ptr m_exception {};
        void quit() noexcept
        { m_coroutineFinished = true; m_loop.quit(); }
    };

    template<Awaitable Awaitable>
    XCoroTask<> runCoroutine(WaitContext & context, Awaitable && awaitable) {
        try { co_await std::forward<Awaitable>(awaitable); }
        catch (...) { context.m_exception = std::current_exception(); }
        context.quit();
    }

    template<typename T, Awaitable Awaitable>
    XCoroTask<> runCoroutine(WaitContext & context, std::optional<T> & result, Awaitable && awaitable) {
        try { result.emplace(co_await std::forward<Awaitable>(awaitable)); }
        catch (...) { context.m_exception = std::current_exception(); }
        context.quit();
    }

    template<typename T, Awaitable Awaitable>
    constexpr T waitFor(Awaitable && awaitable) {

        WaitContext context {};

        auto f { [&context]{
            if (!context.m_coroutineFinished) { (void)context.m_loop.exec(); }
            if (context.m_exception) { std::rethrow_exception(context.m_exception); }
        } };

        if constexpr (std::is_void_v<T>) {
            runCoroutine(context,std::forward<Awaitable>(awaitable));
            f();
            return;
        } else {
            std::optional<T> result {};
            runCoroutine(context,result,std::forward<Awaitable>(awaitable));
            f();
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
