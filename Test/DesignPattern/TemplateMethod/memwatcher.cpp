#include <memwatcher.hpp>

int64_t MemWatcher::watch() {

    m_caches_.push_back(getMem());

    while (m_caches_.size() > 10)
    { m_caches_.erase(m_caches_.cbegin()); }

    int64_t sum {};
    for (auto const & item : m_caches_)
    { sum += item; }

    return sum / static_cast<int64_t>(m_caches_.size());
}
