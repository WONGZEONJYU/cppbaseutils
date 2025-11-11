#include <video.hpp>
#include <statuswaitexamine.hpp>
#include <XMemory/xmemory.hpp>

Video::Video() : m_status_ { XUtils::makeShared<StatusWaitExamine>() }
{
}

void Video::pass() {
    if (auto s{ m_status_->pass() })
    { m_status_.swap(s); }
}

void Video::fail() {
    if (auto s{ m_status_->fail() })
    { m_status_.swap(s); }
}
