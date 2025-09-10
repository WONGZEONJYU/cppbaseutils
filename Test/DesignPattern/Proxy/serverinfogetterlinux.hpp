#ifndef SERVER_INFO_GETTER_LINUX_HPP
#define SERVER_INFO_GETTER_LINUX_HPP 1

#include <serverinfogetterbase.hpp>

class ServerInfoGetterLinux final : public ServerInfoGetterBase {

public:
    ServerInfoGetterLinux() = default;
    ~ServerInfoGetterLinux() override = default;
    ServerInfo getInfo() override;
};

#endif
