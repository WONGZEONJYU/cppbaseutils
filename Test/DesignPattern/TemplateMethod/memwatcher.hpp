#ifndef XUTILS2_MEMWATCHER_HPP
#define XUTILS2_MEMWATCHER_HPP

#include <vector>

class MemWatcher {

protected:
    std::vector<int64_t> m_caches_{};

public:
    virtual ~MemWatcher() = default;
    int64_t watch();
    virtual int64_t getMem() = 0;

protected:
    constexpr MemWatcher() = default;
};

#endif
