#include <note.hpp>
#include <snapshot.hpp>
#include <XMemory/xmemory.hpp>

void Note::append(std::string const & str)
{ m_currentStr_.append(str); }

void Note::backSpace(std::size_t const num) {
    auto const len { m_currentStr_.length() };
    if (num > len) { return; }
    m_currentStr_ = m_currentStr_.substr(0, len - num);
}

std::shared_ptr<Snapshot> Note::createSnapshot() const noexcept
{ return XUtils::makeShared<Snapshot>(m_currentStr_); }

void Note::restoreSnapshot(std::shared_ptr<Snapshot> const & snapshot) {
    if (!snapshot) { return; }
    m_currentStr_.swap(snapshot->m_currentStr_);
}
