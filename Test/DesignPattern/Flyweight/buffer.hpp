#ifndef XUTILS_BUFFER_HPP
#define XUTILS_BUFFER_HPP

#include <memory>

struct Buffer {
    uint8_t * m_buf {};
    size_t m_length {};
    explicit Buffer(size_t );
    ~Buffer();
};

#endif
