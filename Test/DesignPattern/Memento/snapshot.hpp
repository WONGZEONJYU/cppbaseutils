#ifndef XUTILS2_SNAPSHOT_HPP
#define XUTILS2_SNAPSHOT_HPP

#include <string>

struct Snapshot {
    std::string m_currentStr_{};
    constexpr Snapshot() = default;
    constexpr explicit Snapshot(std::string s)
    { m_currentStr_.swap(s); }
};

#endif
