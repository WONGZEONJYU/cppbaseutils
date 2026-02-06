#ifndef XUTILS2_MEMWATCHER_HPP
#define XUTILS2_MEMWATCHER_HPP

#include <vector>
#include <cstdint>

class MemWatcher {

protected:
    std::vector<std::int64_t> m_caches_{};

public:
    virtual ~MemWatcher() = default;
    std::int64_t watch();
    virtual std::int64_t getMem() = 0;

protected:
    constexpr MemWatcher() = default;
};

#endif
