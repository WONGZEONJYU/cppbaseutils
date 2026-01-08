#include <iostream>
#include <thread>
#include <XCoroutine/xcoroutinetask.hpp>
#include <XHelper/xhelper.hpp>
#include "XHelper/xraii.hpp"

#if 0

struct TaskTcb;

struct TaskPromise : XUtils::XPromiseInterface<TaskPromise> {

    auto get_return_object()
    { return std::coroutine_handle<TaskPromise>::from_promise(*this) ; }

    static void return_void() {}
};

struct TaskTcb : XUtils::XCoroutineGenerator<TaskPromise> {

    constexpr explicit(false) TaskTcb(coroutine_handle const h)
        : XCoroutineGenerator { h }
    {}
};

struct awaiter {

     constexpr static bool await_ready() noexcept {
         std::cout << FUNC_SIGNATURE << std::endl;
         return false;
     }

     constexpr static void await_suspend(std::coroutine_handle<>) noexcept {
         std::cout << FUNC_SIGNATURE << std::endl;
     }
     constexpr static void await_resume() noexcept {
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
    co_return;
}

static void testTask1() {

    std::cout << FUNC_SIGNATURE << " begin" << std::endl;

    auto const task1Tcb { task1() };
    //task1Tcb.tryResume();
    // task1Tcb.tryResume();
    getchar();
    std::cout << FUNC_SIGNATURE << " end" << std::endl;
}

#endif

int main() {

    XUtils::XAtomicInteger<uint32_t> m_ref_ {1};
    std::cout << std::boolalpha << m_ref_.deref() << std::endl;

    return 0;
}
