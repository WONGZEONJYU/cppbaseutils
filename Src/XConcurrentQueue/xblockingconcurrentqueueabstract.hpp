#ifndef XUTILS2_X_BLOCKING_CONCURRENT_QUEUE_ABSTRACT_HPP
#define XUTILS2_X_BLOCKING_CONCURRENT_QUEUE_ABSTRACT_HPP 1

#ifndef XUTILS2_X_BLOCKING_CONCURRENT_QUEUE_ABSTRACT_HPP_
#error "xconcurrentqueueabstract is internal header file!"
#endif

#include <XConcurrentQueue/xconcurrentqueue.hpp>
#include <XConcurrentQueue/xlightweightsemaphore.hpp>
#include <memory>
#include <cassert>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace moodycamel {

template<typename T,typename Traits>
class XBlockingConcurrentQueueAbstract {

    template<typename,typename> friend class XBlockingConcurrentQueue;

    using ConcurrentQueue = XConcurrentQueue<T,Traits>;
    using LightweightSemaphore = XLightweightSemaphore;

#if 0
    struct LightweightSemaphoreAllocator;
    using LightweightSemaphorePtr = std::unique_ptr<LightweightSemaphore, LightweightSemaphoreAllocator>;
    friend struct LightweightSemaphoreAllocator;

    struct LightweightSemaphoreAllocator {
        static void cleanup(LightweightSemaphore * const p) noexcept
        { destroy(p); }
        void operator()(LightweightSemaphore * const p) const noexcept
        { cleanup(p); }
        static auto create(){
            return LightweightSemaphorePtr {
                XBlockingConcurrentQueueAbstract::create<LightweightSemaphore>(ssize_t{},static_cast<int>(Traits::MAX_SEMA_SPINS))
                , LightweightSemaphoreAllocator{}
            };
        }
    };
#else
    using LightweightSemaphorePtr = std::unique_ptr<LightweightSemaphore>;
#endif

    ConcurrentQueue m_inner_{};
    LightweightSemaphorePtr m_sema_{};

public:
    using value_type = T;
    using producer_token_t = ConcurrentQueue::producer_token_t;
    using consumer_token_t = ConcurrentQueue::consumer_token_t;

    using index_t = ConcurrentQueue::index_t;
    using size_t = ConcurrentQueue::size_t;
    using ssize_t = std::make_signed_t<size_t>;

    static constexpr auto BLOCK_SIZE{ ConcurrentQueue::BLOCK_SIZE}
                        ,EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD { ConcurrentQueue::EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD }
                        ,EXPLICIT_INITIAL_INDEX_SIZE { ConcurrentQueue::EXPLICIT_INITIAL_INDEX_SIZE }
                        ,IMPLICIT_INITIAL_INDEX_SIZE { ConcurrentQueue::IMPLICIT_INITIAL_INDEX_SIZE }
                        ,INITIAL_IMPLICIT_PRODUCER_HASH_SIZE { ConcurrentQueue::INITIAL_IMPLICIT_PRODUCER_HASH_SIZE };

    static constexpr auto EXPLICIT_CONSUMER_CONSUMPTION_QUOTA_BEFORE_ROTATE { ConcurrentQueue::EXPLICIT_CONSUMER_CONSUMPTION_QUOTA_BEFORE_ROTATE };
    static constexpr auto MAX_SUBQUEUE_SIZE { ConcurrentQueue::MAX_SUBQUEUE_SIZE };

    XBlockingConcurrentQueueAbstract(XBlockingConcurrentQueueAbstract && o) noexcept
    { swap_internal(o); }

    XBlockingConcurrentQueueAbstract& operator=(XBlockingConcurrentQueueAbstract && o) noexcept
    { swap_internal(o); return *this; }

    // Swaps this queue's state with the other's. Not thread-safe.
    // Swapping two queues does not invalidate their tokens, however
    // the tokens that were created for one queue must be used with
    // only the swapped queue (i.e. the tokens are tied to the
    // queue's movable state, not the object itself).
    void swap(XBlockingConcurrentQueueAbstract & other) noexcept
    { swap_internal(other);}

private:

#undef ASSERT_
#define ASSERT_ assert( reinterpret_cast<ConcurrentQueue*>(reinterpret_cast<XBlockingConcurrentQueueAbstract*>(1)) \
                                == std::addressof(reinterpret_cast<XBlockingConcurrentQueueAbstract*>(1)->m_inner_) \
                                && "XBlockingConcurrentQueue must have XConcurrentQueue as its first member");

    explicit XBlockingConcurrentQueueAbstract(size_t const capacity)
        :m_inner_{ capacity }
        ,m_sema_{std::make_unique<XLightweightSemaphore>(0,static_cast<int>(Traits::MAX_SEMA_SPINS) ) }
    { ASSERT_  }

    XBlockingConcurrentQueueAbstract(size_t const minCapacity, size_t const maxExplicitProducers, size_t const maxImplicitProducers)
        :m_inner_ { minCapacity,maxExplicitProducers,maxImplicitProducers }
        ,m_sema_{std::make_unique<XLightweightSemaphore>(0,static_cast<int>(Traits::MAX_SEMA_SPINS) ) }
    { ASSERT_ }

#undef ASSERT_

    template<typename U, typename A1, typename A2>
    static constexpr U * create(A1 && a1, A2 && a2) {
        auto const p { (Traits::malloc)(sizeof(U)) };
        return p ? new (p) U(std::forward<A1>(a1), std::forward<A2>(a2)) : nullptr;
    }

    template<typename U>
    static constexpr void destroy(U * const p)
    { if (p) { p->~U(); } (Traits::free)(p); }

    XBlockingConcurrentQueueAbstract & swap_internal(XBlockingConcurrentQueueAbstract & other) {
        if (this != std::addressof(other)) {
            m_inner_.swap(other.m_inner_);
            m_sema_.swap(other.m_sema_);
        }
        return *this;
    }
};

}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
