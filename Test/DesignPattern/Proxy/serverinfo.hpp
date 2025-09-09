#ifndef XUTILS_SERVERINFO_HPP
#define XUTILS_SERVERINFO_HPP

#include <cstdint>

struct ServerInfo {
    double m_cpu{},m_mem_{};
    int64_t m_createTime{};
    ServerInfo();
    ServerInfo(const ServerInfo& other) = default;
    ServerInfo(ServerInfo&& other) = default;
    ServerInfo& operator=(const ServerInfo& other) = default;
    ServerInfo& operator=(ServerInfo&& other) = default;
    ~ServerInfo() = default;
};

#endif