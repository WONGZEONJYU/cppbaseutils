#include <snapshotmanager.hpp>

void SnapshotManager::push(std::shared_ptr<Snapshot> const & s)
{ m_snapshotStack_.push(s); }

void SnapshotManager::push(std::shared_ptr<Snapshot> && s)
{ m_snapshotStack_.push(std::move(s)); }

std::shared_ptr<Snapshot> SnapshotManager::pop() {
    if (m_snapshotStack_.empty()) { return {}; }
    auto s{ m_snapshotStack_.top() };
    if (!s) { return {}; }
    m_snapshotStack_.pop();
    return s;
}
