#include <serverinfogetterproxy.hpp>
#include <chrono>

ServerInfoGetterProxy::ServerInfoGetterProxy(ServerInfoGetterBase * const o)
:m_getterBase_(o) {}

ServerInfo ServerInfoGetterProxy::getInfo() {

    if (!sm_cache_.m_valid) {
        sm_cache_ = m_getterBase_->getInfo();
        return sm_cache_;
    }

    if (auto const now{ std::chrono::system_clock::now() }
        ; now.time_since_epoch().count() - sm_cache_.m_createTime <= 2000) {
        return sm_cache_;
    }

    sm_cache_ = m_getterBase_->getInfo();
    return sm_cache_;
}
