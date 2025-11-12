#ifndef XUTILS2_MEMWATCHERMACOS_HPP
#define XUTILS2_MEMWATCHERMACOS_HPP

#include <memwatcher.hpp>

class MemWatcherMacos : public MemWatcher
{
public:
    constexpr MemWatcherMacos() = default;
    int64_t getMem() override;
};

#endif
