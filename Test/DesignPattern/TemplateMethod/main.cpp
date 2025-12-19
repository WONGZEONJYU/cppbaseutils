#include <iostream>
#include <memwatchermacos.hpp>
#include <thread>
#include <chrono>
#include <XMemory/xmemory.hpp>

#ifndef X_PLATFORM_WINDOWS
#include <Unix/XSignal/xsignal.hpp>
#endif

int main() {
    bool is_exit {};
#ifndef X_PLATFORM_WINDOWS
    auto const SigHandler { [&is_exit](int,siginfo_t*,void*)
    { is_exit = true; }};
    XUtils::SignalRegister(SIGINT,0,SigHandler);
    XUtils::SignalRegister(SIGTERM,0,SigHandler);
#endif
    auto const watcher{ XUtils::makeUnique<MemWatcherMacos>() };
    for (int i {}; i < 1000 ; i++) {
        if (is_exit) { break; }
        auto const mem{ static_cast<double>(watcher->watch()) / 1073741824.0 };
        std::cout << mem << std::endl;
        using namespace std::chrono;
        std::this_thread::sleep_for(200ms);
    }
    return 0;
}
