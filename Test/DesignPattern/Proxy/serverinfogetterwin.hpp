#ifndef XUTILS_SERVER_INFO_GETTER_WIN_HPP
#define XUTILS_SERVER_INFO_GETTER_WIN_HPP 1

#include <serverinfogetterbase.hpp>

class ServerInfoGetterWin final : public ServerInfoGetterBase {

public:
    ServerInfoGetterWin() = default;
    ~ServerInfoGetterWin() override = default;
    ServerInfo getInfo() override;
};

#endif
