#include <buffer.hpp>
#include <chrono>
#include <iostream>

Buffer::Buffer(size_t const length)
:m_buf(static_cast<uint8_t*>(std::malloc(length)))
,m_length(length)
{
    //std::cout << __PRETTY_FUNCTION__ << std::endl;
}

Buffer::~Buffer() {
    if (m_buf){
        std::free(m_buf);
        m_buf = nullptr;
    }
    m_length = 0;
}
