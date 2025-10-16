#include <buffer.hpp>
#include <bufferpool.hpp>

int main() {
    BufferPool pool{};
    for (int i = 0; i < 100;i++) {
        auto const buffer {pool.getBuffer(1024)};
        pool.returnBuffer(buffer);
    }
    return 0;
}
