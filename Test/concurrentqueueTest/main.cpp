#include <iostream>
#include <XConcurrentQueue/xconcurrentqueue.hpp>
#include <vector>
#include <XConcurrentQueue/xblockingconcurrentqueue.hpp>
#ifdef WIN32
#include <Win/XSignal/xsignal.hpp>
#else
#include <Unix/XSignal/xsignal.hpp>
#endif

int main() {

    XUtils::moodycamel::XConcurrentQueue<int> qq{};

    auto q {std::move(qq) };

    XUtils::moodycamel::ProducerToken ptk{q};

    int a[]{0,1,2,3,4,5,6,7,8,9};

    q.try_enqueue_bulk(ptk,a,10);

    std::vector ret(10,int{});

    decltype(q)::try_dequeue_bulk_from_producer(ptk,ret.data(),10);

    for (auto && i : ret) { std::cout << i << std::endl; }

    std::cout << std::endl;

    XUtils::moodycamel::XBlockingConcurrentQueue<int> bq{};

    int aa[] { 10,20,30,40,50,60,70,80,90,100 };
    bq.try_enqueue_bulk(aa,10);
    bq.try_dequeue_bulk(ret.data(),10);

    for (auto && i : ret) { std::cout << i << std::endl; }

    return 0;
}
