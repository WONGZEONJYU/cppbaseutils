#include <iostream>
#include <XCoroutine/xcoroutinetask.hpp>
#include <XHelper/xhelper.hpp>
#include <XHelper/xtypetraits.hpp>

template<typename T>
struct Awaiter {

    static constexpr bool await_ready() noexcept {
        std::cout << FUNC_SIGNATURE << std::endl;
        return false;
    }
    static constexpr void await_suspend(std::coroutine_handle<> ) noexcept {
        std::cout << FUNC_SIGNATURE << std::endl;
    }
    static constexpr  void await_resume() noexcept {
        std::cout << FUNC_SIGNATURE << std::endl;
    }
};

struct A {
    auto operator co_await() const noexcept
    { return Awaiter<A>{}; }
};

XUtils::XCoroTask<> f1() {
    std::cout << FUNC_SIGNATURE << " begin" << std::endl;
    co_await A{};
    std::cout << FUNC_SIGNATURE << " end" << std::endl;
}

XUtils::XCoroTask<> f2() {
    std::cout << FUNC_SIGNATURE << " begin" << std::endl;
    co_await A{};
    std::cout << FUNC_SIGNATURE << " end" << std::endl;
}

static constexpr void test()
{ XUtils::waitFor(f1()); }

template <typename T>
void ff(T && t) {
    std::cout << XUtils::typeName(std::forward<T>(t)) << std::endl;
}

int main() {

    ff([]{ std::cout << FUNC_SIGNATURE << std::endl; });
    return 0;
}
