#include <iostream>
#include <coroutine1.hpp>
#include <future>
#include <thread>
#include <XHelper/xhelper.hpp>

template<typename T>
struct promise {

    T m_value{};

    using coroutine_handle_type = Generator<promise>;

    auto get_return_object ()
    { return coroutine_handle_type::from_promise(this); }

    static std::suspend_always initial_suspend() noexcept
    { return {}; }

    static auto final_suspend() noexcept {

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

    std::suspend_always yield_value(T v) noexcept
    { m_value = std::move(v); return {}; }

    void unhandled_exception() const noexcept { (void)this;std::terminate(); }

    static coroutine_handle_type get_return_object_on_allocation_failure()
    { return {}; }

    void * operator new(std::size_t const n) noexcept
    { return std::malloc(n); }

    void operator delete(void * const p) noexcept
    { std::free(p); }
};

[[maybe_unused]] static Generator<promise<int>> f1() {
    std::cout << FUNC_SIGNATURE << " begin" << std::endl;
    for (int i{1};i <= 10; ++i) { co_yield i; }
    std::cout << FUNC_SIGNATURE << " end" << std::endl;
    co_return -1;
}

[[maybe_unused]] static void testF1() {
    std::cout << FUNC_SIGNATURE << " begin" << std::endl;
    for (auto const ch{ f1() };!ch.done();) {
        ch();
        std::cout << std::dec << ch.promiseRef()->m_value << std::endl;
    }
    std::cout << FUNC_SIGNATURE << " end" << std::endl;
}

#if 1
template<std::movable T>
struct promise< std::future<T> > {

    using coroutine_handle_type = Generator<promise>;
    using future = std::future<T>;
    future m_finalValue {};

    auto get_return_object()
    { return coroutine_handle_type::from_promise(this); }

    static auto initial_suspend() noexcept
    { return std::suspend_always{}; }

    static auto final_suspend() noexcept {

        struct awaiter {
            static constexpr bool await_ready() noexcept { return {}; }
            static constexpr auto await_suspend(std::coroutine_handle<> const coro)  noexcept
            { if (coro) { while (!coro.done()) { coro.resume(); } } }
            static constexpr void await_resume() noexcept {}
        };

        return awaiter {};
    }

    static void return_void() noexcept{}

    void unhandled_exception() const noexcept { (void)this; std::terminate(); }

    static coroutine_handle_type get_return_object_on_allocation_failure()
    { return {}; }

    void * operator new(std::size_t const n) noexcept
    { return std::malloc(n); }

    void operator delete(void * const p) noexcept
    { std::free(p); }
};
#endif

template <std::movable T>
struct Future {

    T m_value{};

    static constexpr bool await_ready() noexcept { return {}; }

    template<typename coro>
    constexpr auto await_suspend(coro const h) noexcept{
        h.promise().m_finalValue = std::async([this,h]{
            auto const end { m_value };
            for (T i{1}; i < end; ++i) { m_value *= i; }
            if (!h.done()) { h(); }
            return m_value;
        });
    }

    auto await_resume() const noexcept{ return m_value; }

    //void await_transform() = delete;

};

// template <std::movable T>
// auto operator co_await(Future<T> const & f) noexcept{
//     std::cout << FUNC_SIGNATURE << " = " << f.m_value << std::endl;
//     return f;
// }

static Generator<promise<std::future<int>>> f2() {
    std::cout << FUNC_SIGNATURE << " begin" << std::endl;
    Future f { 5} ;
    auto const v { co_await f };
    std::cout << FUNC_SIGNATURE << " Future::m_value = " << v << std::endl;
    std::cout << FUNC_SIGNATURE << " end" << std::endl;
}

[[maybe_unused]] static void testF2()
{
    std::cout << FUNC_SIGNATURE << " begin" << std::endl;
    auto const ch{ f2() };
    while (!ch.done()) {
        ch.resume();
        std::cout << "fuck" << std::endl;
        std::cout << FUNC_SIGNATURE << " wait value = "
            << ch.promiseRef()->m_finalValue.get()
            << std::endl << std::flush;
    }

    std::cout << FUNC_SIGNATURE << " end" << std::endl;
}

int main() {
    //testF1();
    testF2();
    return 0;
}
