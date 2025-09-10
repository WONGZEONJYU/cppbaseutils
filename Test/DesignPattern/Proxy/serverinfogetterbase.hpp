#ifndef SERVER_INFO_GETTER_BASE_HPP
#define SERVER_INFO_GETTER_BASE_HPP 1

#include <serverinfo.hpp>

class ServerInfoGetterBase {
protected:
    ServerInfoGetterBase() = default;
public:
    virtual ~ServerInfoGetterBase()  = default;
    virtual ServerInfo getInfo() = 0;
};

#endif