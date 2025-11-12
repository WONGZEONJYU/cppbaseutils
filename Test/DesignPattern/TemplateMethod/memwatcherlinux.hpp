#ifndef XUTILS2_MEMWATCHERLINUX_HPP
#define XUTILS2_MEMWATCHERLINUX_HPP

#include <memwatcher.hpp>

class MemWatcherLinux final : public MemWatcher {
public:
    constexpr MemWatcherLinux() = default;
    int64_t getMem() override;
};

#endif
