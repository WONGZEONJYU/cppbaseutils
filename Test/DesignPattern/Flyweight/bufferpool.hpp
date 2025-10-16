#ifndef XUTILS_BUFFER_POOL_HPP
#define XUTILS_BUFFER_POOL_HPP 1

#include <map>

struct Buffer;

class BufferPool {
    std::map<size_t,Buffer*> m_cache_{};
public:
    constexpr BufferPool() = default;
    ~BufferPool();
    Buffer * getBuffer(size_t len);
    void returnBuffer(Buffer * buf);
};

#endif
