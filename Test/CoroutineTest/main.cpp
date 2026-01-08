#include <iostream>
#include <thread>
#include <XCoroutine/xcoroutinetask.hpp>
#include <XHelper/xhelper.hpp>
#include "XHelper/xraii.hpp"

struct Awaiter {

    bool await_ready() const noexcept {
        std::cout << FUNC_SIGNATURE << std::endl;
        return false;
    }
    void await_suspend(std::coroutine_handle<> ) noexcept {
        std::cout << FUNC_SIGNATURE << std::endl;
    }
    void await_resume() noexcept {
        std::cout << FUNC_SIGNATURE << std::endl;
    }
};

XUtils::XCoroTask<> f() {
    std::cout << FUNC_SIGNATURE << " begin" << std::endl;
    co_await Awaiter{};
    std::cout << FUNC_SIGNATURE << " end" << std::endl;
}

int main() {
    auto const h{ f()};
    //h.coroutineHandle()->operator()();
    //getchar();

    return 0;
}
