#include <iostream>
//#include <coroutine1.hpp>
#include <XCoroutine/XCoroutine.hpp>
#include <future>
#include <XHelper/xhelper.hpp>

template<typename T>
struct promise {

    constexpr promise() = default;

    constexpr promise(T && v) : m_value { v }{}

    T m_value{};

    using coroutine_handle_type = XUtils::XCoroutineGenerator<promise>;

    auto get_return_object ()
    { return coroutine_handle_type::from_promise(this); }

    static std::suspend_always initial_suspend() noexcept
    { return {}; }

    [[nodiscard]] auto final_suspend() const noexcept {

        struct awaiter {
            static constexpr bool await_ready() noexcept { return {}; }
            static constexpr auto await_suspend(std::coroutine_handle<> const coro)  noexcept
            { if (coro) { while (!coro.done()) { coro.resume(); } } }
            static constexpr void await_resume() noexcept {}
        };

        return awaiter{};
    }

    void return_value(T v) noexcept
    { m_value = std::move(v); }

    auto yield_value(T v) noexcept
    { m_value = std::move(v); return std::suspend_always{};  }

    void unhandled_exception() const noexcept { (void)this;std::terminate(); }

    static coroutine_handle_type get_return_object_on_allocation_failure()
    { return {}; }

    void * operator new(std::size_t const n) noexcept
    { return std::malloc(n); }

    void operator delete(void * const p) noexcept
    { std::free(p); }
};

using promise1 = promise<int>;
using XCoroutine1 = XUtils::XCoroutineGenerator<promise1>;

[[maybe_unused]] static XCoroutine1 f1() {
    std::cout << FUNC_SIGNATURE << " begin" << std::endl;
    for (int i{1};i <= 10; ++i) { co_yield i; }
    std::cout << FUNC_SIGNATURE << " end" << std::endl;
    co_return -1;
}

[[maybe_unused]] static void testF1() {
    std::cout << FUNC_SIGNATURE << " begin" << std::endl;
    for (auto const ch{ f1() };!ch.done();) {
        ch();
        std::cout << std::dec << ch.promise().m_value << std::endl;
    }
    std::cout << FUNC_SIGNATURE << " end" << std::endl;
}

#if 1

template <std::movable T>
struct Future {

    T m_value{};

    static constexpr auto await_ready() noexcept{ return false; }

    template<typename coro>
    constexpr auto await_suspend(coro const h) noexcept{
        h.promise().m_finalValue = std::async([this,h]{
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
struct promise< std::future<T> > {

    using coroutine_handle = XUtils::XCoroutineGenerator<promise>::coroutine_handle;
    using future = std::future<T>;
    future m_finalValue {};

    auto get_return_object()
    { return coroutine_handle::from_promise(*this); }

    static auto initial_suspend() noexcept
    { return std::suspend_always{}; }

    [[nodiscard]] static auto final_suspend() noexcept {
        std::cout << FUNC_SIGNATURE << std::endl;
#if 1
        struct awaiter {
            static constexpr bool await_ready() noexcept { return {}; }
            static constexpr void await_suspend(coroutine_handle const coro)  noexcept{
                while (coro && !coro.done()) {
                    coro.resume();
                    std::cout << FUNC_SIGNATURE << " = " << coro.done() << std::endl;
                }
            }
            static constexpr void await_resume() noexcept {}
        };
        return awaiter {};
#else
        return std::suspend_always {};
#endif
    }

    ~promise() { std::cout << FUNC_SIGNATURE << std::endl; }

    static void return_void() noexcept {}

    void unhandled_exception() const noexcept
    { (void)this; std::terminate(); }

    static coroutine_handle get_return_object_on_allocation_failure()
    { return {}; }

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
        return Future {u};
    }
};

#endif

using promise2 = promise<std::future<int>>;
using Coroutine2 = XUtils::XCoroutineGenerator<promise2>;

static Coroutine2 f2() {
    std::cout << FUNC_SIGNATURE << " begin" << std::endl;
    auto const v { co_await 5 };
    std::cout << FUNC_SIGNATURE << " Future::m_value = " << v << std::endl;
    std::cout << FUNC_SIGNATURE << " end" << std::endl;
}

[[maybe_unused]] static void testF2() {
    std::cout << FUNC_SIGNATURE << " begin" << std::endl;
    auto const ch{ f2() };
    ch();
    ch.promise().m_finalValue.wait();
    std::cout << "wait value = "
        << ch.promise().m_finalValue.get()
        << std::endl;
    std::cout << FUNC_SIGNATURE << " end" << std::endl;
}

int main() {
    testF1();
    std::cout << std::endl << std::endl;
    testF2();
    return 0;
}
