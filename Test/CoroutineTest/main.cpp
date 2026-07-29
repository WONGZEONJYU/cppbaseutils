#include <iostream>
#include <XCoroutine/xcorolazytask.hpp>
#include <XHelper/xhelper.hpp>

template<typename T>
struct Awaiter {

    static constexpr bool await_ready() noexcept {
        std::cout << FUNC_SIGNATURE << std::endl;
        return false;
    }
    static constexpr void await_suspend(std::coroutine_handle<> const h) noexcept {
        std::cout << FUNC_SIGNATURE << std::endl;
        std::cout << XUtils::coroMgrRef().onlineSize() << std::endl;
        h.resume();
    }
    static constexpr  void await_resume() noexcept {
        std::cout << FUNC_SIGNATURE << std::endl;
    }
};

struct A {
    auto operator co_await() const noexcept
    { return Awaiter<A>{}; }
};

XUtils::XCoroTaskVoid f1() {
    std::cout << FUNC_SIGNATURE << " begin" << std::endl;
    std::cout << FUNC_SIGNATURE << " end" << std::endl;
    co_return;
}

XUtils::XCoroTaskVoid f2() {
    std::cout << FUNC_SIGNATURE << " begin" << std::endl;
    std::cout << FUNC_SIGNATURE << " end" << std::endl;
    co_return;
}

XUtils::XCoroTaskVoid f3() {
    std::cout << FUNC_SIGNATURE << " begin" << std::endl;
    std::cout << FUNC_SIGNATURE << " end" << std::endl;
    co_return;
}

XUtils::XCoroTaskVoid fff()
{
    std::cout << FUNC_SIGNATURE << " begin" << std::endl;
    //co_await (f1() | f2 | f3);
    std::cout << FUNC_SIGNATURE << " end" << std::endl;
    co_return;
}

int main() {

#if 0
    XUtils::coroMgrRef().setAllExitCallback([] {
        std::cout << "COROEnd" << std::endl;
    });
    std::cout << XUtils::coroMgrRef().onlineSize() << std::endl;
    f2();
    std::cout << XUtils::coroMgrRef().onlineSize() << std::endl;
#endif

    fff() | f1 | f2 | f3;
    fff() >> f1 >> f2 >> f3;

    return 0;
}
