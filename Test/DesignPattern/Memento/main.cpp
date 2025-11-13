#include <iostream>
#include <note.hpp>
#include <snapshotmanager.hpp>

int main() {
    SnapshotManager snapshotManager{};

    Note note{};

    snapshotManager.push(note.createSnapshot());

    note.append("abcdefg");
    std::cout << note.currentStr() << std::endl;

    snapshotManager.push(note.createSnapshot());

    note.backSpace(3);
    std::cout << note.currentStr() << std::endl;

    note.restoreSnapshot(snapshotManager.pop());
    std::cout << note.currentStr() << std::endl;

    return 0;
}
