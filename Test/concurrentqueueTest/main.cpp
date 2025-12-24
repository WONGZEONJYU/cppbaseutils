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
    auto q { std::move(qq) };
    XUtils::moodycamel::ProducerToken ptk{q};
    XUtils::moodycamel::ConsumerToken csu{q};

#if 1
    {
        auto constexpr in{-100};
        int out{};
        q.enqueue(in);
        q.try_dequeue(out);
        std::cerr << "1 out = " << out << std::endl;
    }

    {
        int out{};
        q.enqueue(-200);
        q.try_dequeue(out);
        std::cerr << "2 out = " << out << std::endl;
    }

    {
        auto constexpr in{-300};
        int out{};
        q.enqueue(ptk,in);
        decltype(q)::try_dequeue_from_producer(ptk,out);
        std::cerr << "3 out = " << out << std::endl;
    }

    {
        auto constexpr in{-400};
        int out{};
        q.try_enqueue(in);
        q.try_dequeue_non_interleaved(out);
        std::cerr << "4 out = " << out << std::endl;
    }

    {
        int out{};
        q.try_enqueue(-500);
        q.try_dequeue_non_interleaved(out);
        std::cerr << "5 out = " << out << std::endl;
    }

    {
        auto constexpr in{-600};
        int out{};
        q.try_enqueue(ptk,in);
        q.try_dequeue_non_interleaved(out);
        std::cerr << "6 out = " << out << std::endl;
    }

    {
        int out{};
        q.try_enqueue(ptk,-700);
        q.try_dequeue(csu,out);
        std::cerr << "7 out = " << out << std::endl;
    }

    {
        int constexpr in[]{-10,-20,-30,-40,-50,-60,-70,-80,-90,-100};
        q.enqueue_bulk(in,std::size(in));
        std::vector out(std::size(in),decltype(*in){});
        q.try_dequeue_bulk(out.data(),out.size());
        for (auto const & i : out)
        { std::cerr << i << "\t"; }
        std::cerr << std::endl;
    }

    {
        int constexpr in[]{ -100,-200,-300,-400,-500,-600,-700,-800,-900,-1000 };
        q.enqueue_bulk(ptk,in,std::size(in));
        std::vector out(std::size(in),decltype(*in){});
        q.try_dequeue_bulk(out.data(),out.size());
        for (auto const & i : out)
            { std::cerr << i << "\t"; }
        std::cerr << std::endl;
    }

    {
        int constexpr in[]{ -110,-210,-310,-410,-510,-610,-710,-810,-910,-1010 };
        q.try_enqueue_bulk(in,std::size(in));
        std::vector out(std::size(in),decltype(*in){});
        q.try_dequeue_bulk(out.data(),out.size());
        for (auto const & i : out)
        { std::cerr << i << "\t"; }
        std::cerr << std::endl;
    }

    {
        int constexpr in[]{ -120,-220,-320,-420,-520,-620,-720,-820,-920,-1020 };
        q.try_enqueue_bulk(ptk,in,std::size(in));
        std::vector out(std::size(in),decltype(*in){});
        decltype(q)::try_dequeue_bulk_from_producer(ptk,out.data(),out.size());
        for (auto const & i : out)
        { std::cerr << i << "\t"; }
        std::cerr << std::endl;
    }
#endif

    {
        std::cerr << "size = " << q.size_approx() << std::endl;
        int constexpr in[1024]{ -130,-230,-330,-430,-530,-630,-730,-830,-930,-1030 };
        // int i{};
        // for (auto && item : in) {
        //     if (!q.try_enqueue(item)) { break; }
        //     ++i;
        // }
        // std::cerr << "i = " << i << std::endl;
        q.try_enqueue_bulk(ptk,in,std::size(in));
        //std::cerr << "try_enqueue_bulk = " << std::boolalpha << q.try_enqueue_bulk(ptk,in,std::size(in)) << std::endl;
        auto const length{q.size_approx()};
        std::cerr << "length = " << length << std::endl;
        std::vector out(length,decltype(*in){});
        std::cerr << "try_dequeue_bulk = " << q.try_dequeue_bulk(out.data(),length) << std::endl;
        for (auto const & item : out) { std::cerr << item << "\t"; }
        std::cerr << "size = " << q.size_approx() << std::endl;
    }

#if 0
    {
        XUtils::moodycamel::XBlockingConcurrentQueue<int> bq{};
        bq.enqueue(10);
        int constexpr in{100};
        bq.try_enqueue(in);
        bq.enqueue(ptk,100);
        bq.enqueue(ptk,in);
        bq.try_enqueue(ptk,100);
        int constexpr ins[]{0,-1,-2,-3};
        bq.try_enqueue_bulk(ptk,ins,std::size(ins));

        int ret{};
        bq.try_dequeue(ret);
#if 1
        int p[10]{};
        bq.wait_dequeue_bulk(csu,p,10);
        bq.wait_dequeue_bulk_timed(csu,p,std::ranges::size(p),10);
        bq.try_dequeue_bulk(csu,p,std::ranges::size(p));
        bq.try_dequeue_bulk(p,std::ranges::size(p));
        //int ret1{};
        bq.wait_dequeue_timed(csu,ret,10);
        //bq.wait_dequeue(csu,ret);
        bq.try_dequeue(ret);
#endif
    }
#endif

    return 0;
}
