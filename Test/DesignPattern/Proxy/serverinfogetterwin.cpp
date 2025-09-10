#include <serverinfogetterwin.hpp>

ServerInfo ServerInfoGetterWin::getInfo() {
    ServerInfo info{};
    info.m_cpu = 0.6;
    info.m_mem = 0.3;
    info.m_valid = true;
    return info;
}
