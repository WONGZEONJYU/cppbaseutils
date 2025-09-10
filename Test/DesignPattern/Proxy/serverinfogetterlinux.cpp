#include <serverinfogetterlinux.hpp>

ServerInfo ServerInfoGetterLinux::getInfo() {
    ServerInfo info{};
    info.m_cpu = 0.7;
    info.m_mem = 0.53;
    info.m_valid = true;
    return info;
}
