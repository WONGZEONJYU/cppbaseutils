#include <iostream>
#include <taskthread.hpp>
#include <XGlobal/xclasshelpermacros.hpp>

#ifndef X_PLATFORM_WINDOWS
#include <Unix/XSignal/xsignal.hpp>
#endif

class MyRunnable : public Runnable {

public:
    void run() override {
        std::cout << FUNC_SIGNATURE << std::endl;
    }
};

class MyTask : public Runnable
{
public:
    void run() override {
        std::cout << FUNC_SIGNATURE << std::endl;
    }
};

int main() {

    bool is_exit{};

#ifndef X_PLATFORM_WINDOWS
    auto const sigterm{ XUtils::SignalRegister(SIGTERM,0,
        [&is_exit](int const ,siginfo_t * const ,void * const &) noexcept -> void {
            is_exit = true;
    })} ;

    auto const sigint{ XUtils::SignalRegister(SIGINT,0,
        [&is_exit](int const ,siginfo_t * ,void * ) noexcept ->void {
        is_exit = true;
    })};

    std::cout << "current pid:" << getpid() << std::endl;
#endif

    TaskThread taskThread;

    const auto task0 { std::make_shared<MyRunnable>() };
    const auto task1 { std::make_shared<MyRunnable>() };
    const auto task2 { std::make_shared<MyTask>() };
    const auto task3 { std::make_shared<MyTask>() };

    taskThread.putTask(task0);
    taskThread.putTask(task1);
    taskThread.putTask(task2);
    taskThread.putTask(task3);

    while (!is_exit) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 0;
}
