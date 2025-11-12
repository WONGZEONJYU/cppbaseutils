#ifndef XUTILS2_MEMWATCHERWINDOWS_HPP
#define XUTILS2_MEMWATCHERWINDOWS_HPP

#include <memwatcher.hpp>

class MemWatcherWindows final : public MemWatcher {
public:
    constexpr MemWatcherWindows() = default;
    int64_t getMem() noexcept override;
};

#endif
