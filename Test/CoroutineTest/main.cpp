#include <iostream>
#include <XCoroutine/xcoroutine.hpp>
#include <future>
#include <thread>
#include <XHelper/xhelper.hpp>

template<typename T>
struct promise : XUtils::XPromiseAbstract {

    constexpr promise()
    {
        std::cout << FUNC_SIGNATURE << std::endl;
    }

    constexpr  promise(T && v) : m_value { v }
    {
        std::cout << FUNC_SIGNATURE << std::endl;
    }

    ~promise() override
    { std::cout << FUNC_SIGNATURE << std::endl; }

    T m_value{};

    using CoroutineGenerator = XUtils::XCoroutineGenerator<promise>;
    using coroutine_handle = CoroutineGenerator::coroutine_handle;

    auto get_return_object ()
    { return coroutine_handle::from_promise(*this); }

    [[nodiscard]] auto initial_suspend() const noexcept
    { onSuspended(); return std::suspend_always {}; }

    auto return_value(T v) noexcept
    { m_value = std::move(v); onSuspended(); }

    template<std::convertible_to<T> From>
    auto yield_value(From && from) noexcept {
        m_value = std::forward<From>(from);
        return std::suspend_always{};
    }

    static auto unhandled_exception() noexcept{ std::terminate(); }

    static auto get_return_object_on_allocation_failure()
    { return coroutine_handle{}; }

    void * operator new(std::size_t const n) noexcept
    {
        std::cout << FUNC_SIGNATURE << " n = " << n << std::endl;
        return std::malloc(n);
    }

    void operator delete(void * const p) noexcept
    { std::free(p); }
};

struct Task1 : XUtils::XCoroutineGenerator<promise<int>> {
    constexpr explicit(false) Task1(coroutine_handle const h = {})
        : XCoroutineGenerator { h } {}
};

[[maybe_unused]] static Task1 f1() {
    std::cout << FUNC_SIGNATURE << " begin" << std::endl;
    for (int i{1};i <= 10; ++i) { co_yield i; }
    std::cout << FUNC_SIGNATURE << " end" << std::endl;
    co_return -1;
}

[[maybe_unused]] static void testF1() {
    std::cout << FUNC_SIGNATURE << " begin" << std::endl;

    auto const task1{ f1() };
    Task1 t1{},t2 {};
    std::cout << FUNC_SIGNATURE << " (ch == t1) " << std::boolalpha << (t2 == t1) << std::endl;

    for (;!task1.done();) {
        static_cast<bool>(task1());
        std::cout << FUNC_SIGNATURE << " "
            << std::dec << task1.promise().m_value << std::endl;
    }
    std::cout << FUNC_SIGNATURE << " end" << std::endl;
}

#if 1

template <std::movable T>
struct FutureAwait {

    T m_value {};

    static constexpr auto await_ready() noexcept
    { return false; }

    template<typename coro>
    constexpr auto await_suspend(coro && h) noexcept{
        auto && promise { h.promise() };
        promise.m_finalValue = std::async([this,h]{
            auto const end { m_value };
            for (T i{1}; i < end; ++i) { m_value *= i; }
            if (!h.done()) { h.resume(); }
            return m_value;
        });
    }

    [[nodiscard]] auto await_resume() const noexcept
    { return m_value; }
};

template<std::movable T>
struct promise< std::future<T> > : XUtils::XPromiseAbstract {

    using CoroutineGenerator = XUtils::XCoroutineGenerator<promise>;
    using coroutine_handle = CoroutineGenerator::coroutine_handle;

    using future = std::future<T>;
    future m_finalValue {};

    auto get_return_object()
    { return coroutine_handle::from_promise(*this); }

    static auto initial_suspend() noexcept
    { return std::suspend_always{}; }

    ~promise() override { std::cout << FUNC_SIGNATURE << std::endl; }

    static void return_void() noexcept {}

    void unhandled_exception() const noexcept
    { (void)this; std::terminate(); }

    static auto get_return_object_on_allocation_failure()
    { return coroutine_handle{}; }

    void * operator new(std::size_t const n) noexcept {
        std::cout << FUNC_SIGNATURE << std::endl;
        return std::malloc(n);
    }

    void operator delete(void * const p) noexcept {
        std::cout << FUNC_SIGNATURE << std::endl;
        std::free(p);
    }

    template<typename U>
    static auto await_transform(U && u) {
        std::cout << FUNC_SIGNATURE << typeid(u).name() << std::endl;
        return FutureAwait {u};
    }
};

struct Task2 : XUtils::XCoroutineGenerator<promise<std::future<int>>> {
    constexpr Task2( coroutine_handle const h = {}) noexcept
        : XCoroutineGenerator { h } {  }
};

#endif

static Task2 f2() {
    std::cout << FUNC_SIGNATURE << " begin" << std::endl;
    auto const v { co_await 5 };
    std::cout << FUNC_SIGNATURE << " Future::m_value = " << v << std::endl;
    std::cout << FUNC_SIGNATURE << " end" << std::endl;
}

[[maybe_unused]] static void testF2() {
    std::cout << FUNC_SIGNATURE << " begin" << std::endl;
    auto const ch{ f2() };
    ch.tryResume();
    ch.promise().m_finalValue.wait();
    std::cout << "wait value = "
        << ch.promise().m_finalValue.get()
        << std::endl;
    std::cout << FUNC_SIGNATURE << " end" << std::endl;
}

int main() {
    testF1();
    std::cout << std::endl << std::endl;
    //testF2();
    return 0;
}
