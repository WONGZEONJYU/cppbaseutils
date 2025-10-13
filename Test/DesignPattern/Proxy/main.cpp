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
    auto const sigterm{ XUtils::SignalRegister(SIGTERM,0,[&is_exit]() noexcept{
        is_exit = true;
    })};

    auto const sigint{ XUtils::SignalRegister(SIGINT,0,[&is_exit]() noexcept{
    is_exit = true;
})};

#endif
    while (!is_exit) {
        ServerInfoGetterLinux linux{};
        ServerInfoGetterWin win{};

        ServerInfoGetterProxy getter(&win);
        const auto info{getter.getInfo()};
        std::cerr << info.m_cpu << '\n';
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }
    return 0;
}
