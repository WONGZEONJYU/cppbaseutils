#include <serverinfo.hpp>
#include <chrono>

ServerInfo::ServerInfo() {
    auto const start{ std::chrono::high_resolution_clock::now() };
    m_createTime = start.time_since_epoch().count();
}
