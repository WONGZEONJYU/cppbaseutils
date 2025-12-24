#include <iostream>
#include <vector>
#include <XConcurrentQueue/xconcurrentqueueproxy.hpp>
#ifdef WIN32
#include <Win/XSignal/xsignal.hpp>
#else
#include <Unix/XSignal/xsignal.hpp>
#endif

int main()
{
#if 1
    {
        XUtils::moodycamel::XConcurrentQueueProxy<int> qq{};
        auto q { std::move(qq) };
        XUtils::moodycamel::ProducerToken ptk{q.m_q};
        XUtils::moodycamel::ConsumerToken csu{qq.m_q};
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
        q.try_dequeue_from_producer(ptk,out);
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
        q.try_dequeue_bulk_from_producer(ptk,out.data(),out.size());
        for (auto const & i : out)
        { std::cerr << i << "\t"; }
        std::cerr << std::endl;
    }
    #endif


    {
        std::cerr << "size = " << q.size() << std::endl;
        int constexpr in[1024]{ -130,-230,-330,-430,-530,-630,-730,-830,-930,-1030 };
        // int i{};
        // for (auto && item : in) {
        //     if (!q.try_enqueue(item)) { break; }
        //     ++i;
        // }
        // std::cerr << "i = " << i << std::endl;
        q.try_enqueue_bulk(ptk,in,std::size(in));
        //std::cerr << "try_enqueue_bulk = " << std::boolalpha << q.try_enqueue_bulk(ptk,in,std::size(in)) << std::endl;
        auto const length{q.length()};
        std::cerr << "length = " << length << std::endl;
        std::vector out(length,decltype(*in){});
        std::cerr << "try_dequeue_bulk = " << q.try_dequeue_bulk(out.data(),length) << std::endl;
        std::cerr << "size = " << q.size() << std::endl;
        for (auto const & item : out) { std::cerr << item << "\t"; }
        std::cerr << std::endl;
    }
    }
#endif

#if 1
    {
        XUtils::moodycamel::XBlockingConcurrentQueueProxy<int> bbq{};
        XUtils::moodycamel::swap(bbq,bbq);

        auto bq { std::move(bbq) };
        XUtils::moodycamel::ProducerToken ptk {bq.m_q};
        XUtils::moodycamel::ConsumerToken csu{bq.m_q};

        bq.enqueue(-100000);
        std::cerr << "size = " << bq.size() << std::endl;
        int v {};
        bq.wait_dequeue(v);
        std::cerr << "v = " << v << std::endl;

        int constexpr ins[]{-150,-151,-152,-153,-154,-155,-156,-157,-158,-159};
        bq.try_enqueue_bulk(ins,std::size(ins));
        bq.try_enqueue_bulk(ins,std::size(ins));

        std::cerr << "length = " << bq.length() << std::endl;

        std::vector outs(bq.length(),decltype(*ins){});
        using namespace std::chrono;
        bq.wait_dequeue_bulk(csu,outs.data(),bq.length());
        for (auto const & i : outs) { std::cerr << i << "\t"; }
        std::cerr << std::endl;
    }
#else


#endif

    return 0;
}
