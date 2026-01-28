// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include <coroutine>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <exception>

#include <QCoreApplication>
#include <QMetaObject>
#include <QTimer>

struct FutureString : QObject {
    Q_OBJECT
    bool m_ready_ {};
    QString m_str_ {};

public:
    Q_IMPLICIT FutureString(QString const & str)
        : m_str_{str}
    {
        using namespace std::chrono_literals;
        QTimer::singleShot(1s, this, [this]{
            m_ready_ = true;
            Q_EMIT ready();
        });
    }

    constexpr bool isReady() const { return m_ready_; }

    QString result() const { return m_str_; }

    Q_SIGNAL void ready();
};

// Awaiter is a concept that provides the await_* methods below, which are used by the
// co_await expression.
// Type is Awaitable if it supports the `co_await` operator.
//
// When compiler sees a `co_await <expr>`, it first tries to obtain an Awaitable type for
// the expression result result type:
//  - first by checking if the current coroutine's promise type has `await_transform()`
//    that for given type returns an Awaitable
//  - if it does not have await_transform, it treats the result type as awaitable.
// Thus, if the current promise type doesn't have compatible `await_transform()` and the
// type itself is not Awaitable, it cannot be `co_await`ed.
//
// If the Awaitable object has `operator co_await` overload, it calls it to obtain the
// Awaiter object. Otherwise the Awaitable object is used as an Awaiter.
//
class FutureStringAwaiter {
    std::shared_ptr<FutureString> m_future_{};
public:
    Q_IMPLICIT FutureStringAwaiter(std::shared_ptr<FutureString> const & value) noexcept
        : m_future_{value}
    { std::cout << "FutureStringAwaiter constructed." << std::endl; }

    ~FutureStringAwaiter() { std::cout << "FutureStringAwaiter destroyed." << std::endl; }

    // Called by compiler when starting co_await to check whether the awaited object is by any
    // chance already ready, so that we could avoid the suspend-resume dance.
    bool await_ready() const noexcept {
        std::cout << "FutureStringAwaiter::await_ready() called." << std::endl;
        return m_future_->isReady();
    }
    // Called to tell us that the awaiting coroutine was suspended.
    // We use the awaitingCoroutine handle to resume the suspended coroutine once the
    // co_awaited coroutine is finished.
    void await_suspend(std::coroutine_handle<> const h) noexcept {
        std::cout << "FutureStringAwaiter::await_suspend() called." << std::endl;
        QObject::connect(m_future_.get(), &FutureString::ready,
                         [h]{ h.resume(); });
    }
    // Called when the co_awaiting coroutine is resumed. Returns result of the
    // co_awaited expression.
    QString await_resume() const noexcept {
        std::cout << "FutureStringAwaiter::await_resume() called." << std::endl;
        return m_future_->result();
    }
};

class FutureStringAwaitable {
    std::shared_ptr<FutureString> m_future_{};
public:
    Q_IMPLICIT FutureStringAwaitable(std::shared_ptr<FutureString> const & value) noexcept
        : m_future_{value}
    { std::cout << "FutureStringAwaitable constructed." << std::endl; }

    ~FutureStringAwaitable()
    { std::cout << "FutureStringAwaitable destroyed." << std::endl; }

    auto operator co_await() const noexcept{
        std::cout << "FutureStringAwaitable::operator co_await() called." << std::endl;
        return FutureStringAwaiter {m_future_};
    }
};

struct VoidPromise {

    Q_IMPLICIT VoidPromise()
    { std::cout << "VoidPromise constructed." << std::endl; }

    ~VoidPromise()
    { std::cout << "VoidPromise destroyed." << std::endl; }

    struct promise_type {

        Q_IMPLICIT promise_type()
        { std::cout << "VoidPromise::promise_type constructed." << std::endl; }

        ~promise_type()
        { std::cout << "VoidPromise::promise_type destroyed." << std::endl; }

        // Says whether the coroutine body should be executed immediately (`suspend_never`)
        // or whether it should be executed only once the coroutine is co_awaited.
        static constexpr auto initial_suspend() noexcept
        { return std::suspend_never {}; }
        // Says whether the coroutine should be suspended after returning a result
        // (`suspend_always`) or whether it should just end and the frame pointer and everything
        // should be destroyed.
        static constexpr auto final_suspend() noexcept
        { return std::suspend_never {}; }

        // Called by the compiler during initial coroutine setup to obtain the object that
        // will be returned from the coroutine when it is suspended.
        // Sicne this is a promise type for VoidPromise, we return a VoidPromise.
        static auto get_return_object() noexcept{
            std::cout << "VoidPromise::get_return_object() called." << std::endl;
            return VoidPromise{};
        }

        // Called by the compiler when an exception propagates from the coroutine.
        // Alternatively, we could declare `set_exception()` which the compiler would
        // call instead to let us handle the exception (e.g. propagate it)
        static void unhandled_exception() noexcept
        { std::terminate(); }

        // The result of the promise. Since our promise is void, we must implement `return_void()`.
        // If our promise would be returning a value, we would have to implement `return_value()`
        // instead.
        static constexpr void return_void() noexcept {}

        static auto await_transform(std::shared_ptr<FutureString> const & future)noexcept {
            std::cout << "VoidPromise::await_transform called." << std::endl;
            return FutureStringAwaitable{future};
        }
    };
};

static std::shared_ptr<FutureString> regularFunction()
{ return std::make_shared<FutureString>(QStringLiteral("Hello World!")); }

// This function co_awaits, therefore it's a co-routine and must
// have a promise type to return to the caller.
static VoidPromise myCoroutine() {
    // 1. Compiler creates a new coroutine frame `f`
    // 2. Compiler obtains a return object from the promise.
    //    The promise is of type `std::coroutine_traits<CurrentFunctionReturnType>::promise_type`,
    //    which is `CurrentFunctionReturnType::promise_type` (if there is no specialization for
    //    `std::coroutine_traits<CurrentFunctionReturnType>`)
    // 3. Compiler starts execution of the coroutine body by calling `resume()` on the
    //    current coroutine's std::coroutine_handle (obtained from the promise by
    //    `std::coroutine_handle<decltype(f->promise)>::from_promise(f->promise)

    std::cout << "myCoroutine() started." << std::endl;
    auto const result { co_await regularFunction()};
    std::cout << "Result successfully co_await-ed: " << result.toStdString() << std::endl;

    qApp->quit();
}

int main(int argc, char **argv) {

    QCoreApplication app{argc, argv};
    QMetaObject::invokeMethod(&app, myCoroutine);
    QTimer t{};
    t.callOnTimeout(std::addressof(app),[]()noexcept{ std::cout << "Tick" << std::endl; });
    using namespace std::chrono_literals;
    t.start(100ms);
    return app.exec();
}

#include "await-async-string.moc"
