#include <iostream>
#include <thread>
#include <XCoroutine/xcoroutine.hpp>
#include <XHelper/xhelper.hpp>
#include "XHelper/xraii.hpp"

struct TaskPromise : XUtils::XPromiseAbstract {

    auto get_return_object()
    { return std::coroutine_handle<TaskPromise>::from_promise(*this); }

    static auto initial_suspend() {
        return std::suspend_always{};
    }

    static void return_void() {}
    static void unhandled_exception() {}
};

struct TaskTcb : XUtils::XCoroutineGenerator<TaskPromise> {

    constexpr explicit(false) TaskTcb(coroutine_handle const h)
        : XCoroutineGenerator { h }
    {}
};

struct awaiter {

     constexpr bool await_ready() const noexcept {
         std::cout << FUNC_SIGNATURE << std::endl;
         return false;
     }
     constexpr void await_suspend(std::coroutine_handle<>) const noexcept {
         std::cout << FUNC_SIGNATURE << std::endl;
        // std::this_thread::sleep_for(std::chrono::seconds { 5 });
     }
     constexpr void await_resume() const noexcept {
         std::cout << FUNC_SIGNATURE << std::endl;
     }
};


static TaskTcb task1() {
    std::cout << FUNC_SIGNATURE << " begin" << std::endl;

    XUtils::X_RAII const r{ [] {
        std::cout << FUNC_SIGNATURE << " begin" << std::endl;
    },[] {
        std::cout << FUNC_SIGNATURE << " end" << std::endl;
    } };

    co_await awaiter{};

    std::cout << FUNC_SIGNATURE << " end" << std::endl;
}

static void testTask1() {

    std::cout << FUNC_SIGNATURE << " begin" << std::endl;

    auto const task1Tcb { task1() };
    task1Tcb.tryResume();
    task1Tcb.tryResume();

    std::cout << FUNC_SIGNATURE << " end" << std::endl;
}

int main() {
    testTask1();
    return 0;
}
