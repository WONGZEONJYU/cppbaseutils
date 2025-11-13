#ifndef XUTILS2_SNAPSHOTMANAGER_HPP
#define XUTILS2_SNAPSHOTMANAGER_HPP

#include <memory>
#include <stack>

struct Snapshot;

class SnapshotManager {

protected:
    std::stack<std::shared_ptr<Snapshot>> m_snapshotStack_{};

public:
    void push(std::shared_ptr<Snapshot> const & );
    void push(std::shared_ptr<Snapshot> && );

    std::shared_ptr<Snapshot> pop();

    constexpr SnapshotManager() = default;
    virtual ~SnapshotManager() = default;
};

#endif
