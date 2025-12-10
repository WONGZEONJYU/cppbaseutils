#ifndef XUTILS2_NOTE_HPP
#define XUTILS2_NOTE_HPP

#include <string>
#include <memory>

struct Snapshot;

class Note {

    std::string m_currentStr_{};

public:
    constexpr Note() = default;
    virtual ~Note() = default;
    void append(std::string const & );
    void backSpace(std::size_t);
    [[nodiscard]] constexpr const std::string & currentStr() const noexcept
    { return m_currentStr_; }
    [[nodiscard]] std::shared_ptr<Snapshot> createSnapshot() const noexcept;
    void restoreSnapshot(std::shared_ptr<Snapshot> const & snapshot);
};

#endif
