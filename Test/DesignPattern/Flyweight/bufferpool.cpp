#include <bufferpool.hpp>
#include <buffer.hpp>
#include <ranges>

BufferPool::~BufferPool() {
    for (auto const &item : m_cache_ | std::ranges::views::values) {
        auto const p { item };
        delete p;
    }
}

Buffer* BufferPool::getBuffer(size_t const len)
{
    if ( auto const it { m_cache_.find(len) }
        ;m_cache_.end() != it)
    {
        auto const p{ it->second };
        m_cache_.erase(it);
        return p;
    }
    size_t length {};
    while (len > length) { length += 128; }
    length = !length ? 128 : length;
    return new Buffer(length);
}

void BufferPool::returnBuffer(Buffer * const buf)
{ m_cache_[buf->m_length] = buf; }
