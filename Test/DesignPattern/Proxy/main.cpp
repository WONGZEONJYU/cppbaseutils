#include <iostream>
#include <serverinfogetterlinux.hpp>
#include <serverinfogetterwin.hpp>
#include <serverinfogetterproxy.hpp>
#include <thread>

#if !defined(_WIN32) && !defined(_WIN64)
#include <XSignal/xsignal.hpp>
#endif

int main() {
    bool is_exit {};
#if !defined(_WIN32) && !defined(_WIN64)
    auto const sigterm{ XUtils::SignalRegister(SIGTERM,0,
        [&is_exit](int const ,siginfo_t * const ,void * const &) noexcept -> void {
            is_exit = true;
    })} ;

    auto const sigint{ XUtils::SignalRegister(SIGINT,0,
        [&is_exit](int const ,siginfo_t * ,void * ) noexcept ->void {
        is_exit = true;
    })};

    auto const sigkill{ XUtils::SignalRegister(SIGKILL,0,
        [&is_exit](int const &,siginfo_t * const ,void * ) noexcept ->void {
        is_exit = true;
    })};

    auto const sigusr1{ XUtils::SignalRegister(SIGUSR1,0,
        [](int const ,const siginfo_t * const info,void * ) noexcept ->void {
            std::cerr << info->si_code << std::endl;
    })};

    std::cout << "current pid:" << getpid() << std::endl;
#endif

#if 1
    while (!is_exit) {
        ServerInfoGetterLinux linux{};
        ServerInfoGetterWin win{};

        ServerInfoGetterProxy getter(&win);
        const auto info{getter.getInfo()};
        std::cout << info.m_cpu << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
#endif
    std::cout << "main() returning now\n";
    return 0;
}
