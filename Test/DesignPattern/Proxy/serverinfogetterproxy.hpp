#ifndef SERVER_INFO_GETTER_PROXY_HPP
#define SERVER_INFO_GETTER_PROXY_HPP 1

#include <serverinfogetterbase.hpp>
#include <serverinfo.hpp>

class ServerInfoGetterProxy final : public ServerInfoGetterBase {
    inline static ServerInfo sm_cache_ {};
    ServerInfoGetterBase * m_getterBase_{};
public:
    explicit ServerInfoGetterProxy(ServerInfoGetterBase * );
    ~ServerInfoGetterProxy() override = default;
    ServerInfo getInfo() override;
};

#endif
