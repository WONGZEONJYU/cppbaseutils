#include <iostream>
#include <XContainer/xconcurrentqueue.hpp>
#include <vector>

int main() {

    XUtils::moodycamel::ConcurrentQueue<int> q {};

    int a[]{0,1,2,3,4,5,6,7,8,9};
    q.try_enqueue_bulk(a,10);

    std::vector ret(10,int{});

    q.try_dequeue_bulk(ret.data(),10);

    for (auto && i : ret)
    {
        std::cout << i << std::endl;
    }

    std::cout << "Hello world!" << std::endl;
    return 0;
}
