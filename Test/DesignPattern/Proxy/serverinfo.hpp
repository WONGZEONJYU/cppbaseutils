#ifndef XUTILS_SERVERINFO_HPP
#define XUTILS_SERVERINFO_HPP

#include <memory>

struct ServerInfo final {
    double m_cpu{},m_mem{};
    int64_t m_createTime{};
    bool m_valid{};
    ServerInfo();
    ServerInfo(const ServerInfo& other) = default;
    ServerInfo(ServerInfo&& other) = default;
    ServerInfo& operator=(const ServerInfo & other) = default;
    ServerInfo& operator=(ServerInfo && other) = default;
    ~ServerInfo() = default;
};

#endif
