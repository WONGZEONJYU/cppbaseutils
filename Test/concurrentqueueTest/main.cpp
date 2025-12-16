#include <iostream>
#include <XContainer/xconcurrentqueue.hpp>

int main() {

    XUtils::moodycamel::ConcurrentQueue<int> q{};

    q.enqueue(1);
    q.try_enqueue(2);

    int v{};
    q.try_dequeue(v);

    std::cout << "Hello world!" << std::endl;
    return 0;
}
