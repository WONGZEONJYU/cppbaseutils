#include <iostream>
#include <XConcurrentQueue/xconcurrentqueue.hpp>
#include <vector>
#include <XConcurrentQueue/lightweightsemaphore.hpp>

int main() {

    XUtils::moodycamel::XConcurrentQueue<int> qq{};

    auto q {std::move(qq) };

    XUtils::moodycamel::ProducerToken ptk{q};

    XUtils::moodycamel::ConsumerToken ptk2{q};

    int a[]{0,1,2,3,4,5,6,7,8,9};

    q.try_enqueue_bulk(ptk,a,10);

    std::vector ret(10,int{});

    decltype(q)::try_dequeue_bulk_from_producer(ptk,ret.data(),10);

    for (auto && i : ret)
    {
        std::cout << i << std::endl;
    }

    XUtils::moodycamel::XLightweightSemaphore semaphore{};
    semaphore.wait(2*1000*1000);

    std::cout << "Hello world!" << std::endl;
    return 0;
}
