#ifndef XUTILS2_X_CONCURRENT_QUEUE_PROXY_HPP
#define XUTILS2_X_CONCURRENT_QUEUE_PROXY_HPP 1

#pragma once

#include <XConcurrentQueue/xconcurrentqueue.hpp>
#include <XConcurrentQueue/xblockingconcurrentqueue.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace moodycamel {

    template<typename T
			,typename Traits = XConcurrentQueueDefaultTraits
			,typename QueueType = XConcurrentQueue<T,Traits>
	> struct XConcurrentQueueProxy;

	template<typename T
			,typename Traits = XConcurrentQueueDefaultTraits
			,typename QueueType = XBlockingConcurrentQueue<T,Traits>
	> struct XBlockingConcurrentQueueProxy;

	template<typename T,typename Traits,typename QueueType>
	struct XConcurrentQueueProxy {

		using ConcurrentQueue = QueueType;
		using value_type = ConcurrentQueue::value_type;
		using size_t = ConcurrentQueue::size_t;
		using index_t = ConcurrentQueue::index_t;
		using producer_token_t = ConcurrentQueue::producer_token_t;
		using consumer_token_t = ConcurrentQueue::consumer_token_t;

		ConcurrentQueue m_q {};

	protected:
		XAtomicInteger<typename ConcurrentQueue::size_t> m_count {};

	public:
		template<typename ...Args>
		constexpr explicit XConcurrentQueueProxy(Args && ...args)
			: m_q { std::forward<Args>(args)... }
		{}

		constexpr size_t size() const noexcept { return m_count.loadRelaxed(); }
		constexpr size_t length() const noexcept { return m_count.loadRelaxed(); }
		constexpr size_t size_approx() const noexcept { return m_q.size_approx(); }
		[[nodiscard]] constexpr bool empty() const noexcept { return !m_count.loadRelaxed(); }
		[[nodiscard]] constexpr bool isEmpty() const noexcept { return !m_count.loadRelaxed(); }

#undef CHECK_NOEXCEPT_
#undef NOEXCEPT_
#define CHECK_NOEXCEPT_(Class,fn) \
		noexcept( noexcept( std::declval<Class &>().fn( ( std::declval<Args && >() )... ) ) )
#define NOEXCEPT_(fn) CHECK_NOEXCEPT_(ConcurrentQueue,fn)

#define MAKE_ENQUEUE_FUNC(en_fn) \
		template<typename ...Args> \
		constexpr bool en_fn(Args && ...args) NOEXCEPT_(en_fn) { \
			auto const ret { m_q.en_fn(std::forward<Args>(args)...) }; \
			if ((details::likely)(ret)) { m_count.ref(); } \
			return ret; \
		}

		MAKE_ENQUEUE_FUNC(enqueue)
		MAKE_ENQUEUE_FUNC(try_enqueue)

#undef MAKE_ENQUEUE_FUNC

#define MAKE_ENQUEUE_BULK_FUNC(en_bulk_fn) \
		template<typename ...Args> \
		constexpr bool en_bulk_fn(Args && ...args) NOEXCEPT_(en_bulk_fn) { \
			using Tuple = std::tuple<Args...>; \
			auto constexpr lastInx { std::tuple_size_v<Tuple> - 1 }; \
			auto const len { std::get<lastInx>(Tuple{std::forward<Args>(args)...}) }; \
			auto const ret { m_q.en_bulk_fn(std::forward<Args>(args)...) }; \
			if ((details::likely)(ret)) { m_count.fetchAndAddOrdered(len); } \
			return ret; \
		}

		MAKE_ENQUEUE_BULK_FUNC(enqueue_bulk)
		MAKE_ENQUEUE_BULK_FUNC(try_enqueue_bulk)

#undef MAKE_ENQUEUE_BULK_FUNC

#define MAKE_DEQUEUE_FUNC(de_fn) \
		template<typename ...Args> \
		constexpr bool de_fn(Args && ...args) NOEXCEPT_(de_fn) { \
			auto const ret { m_q.de_fn(std::forward<Args>(args)...) }; \
			if ((details::likely)(ret)) { m_count.deref(); } \
			return ret; \
		}

		MAKE_DEQUEUE_FUNC(try_dequeue)
		MAKE_DEQUEUE_FUNC(try_dequeue_non_interleaved)

#undef MAKE_DEQUEUE_FUNC

		template<typename ...Args>
		constexpr size_t try_dequeue_bulk(Args && ...args) NOEXCEPT_(try_dequeue_bulk) {
			auto const ret { m_q.try_dequeue_bulk(std::forward<Args>(args)...) };
			m_count.fetchAndSubOrdered(ret);
			return ret;
		}

		template<typename ...Args>
		constexpr bool try_dequeue_from_producer(Args && ...args)
			NOEXCEPT_(try_dequeue_from_producer)
		{
			auto const ret { ConcurrentQueue::try_dequeue_from_producer(std::forward<Args>(args)...) };
			if ((details::likely)(ret)) { m_count.deref(); }
			return ret;
		}

		template<typename ...Args>
		constexpr size_t try_dequeue_bulk_from_producer(Args && ...args)
			NOEXCEPT_(try_dequeue_bulk_from_producer)
		{
			auto const ret { ConcurrentQueue::try_dequeue_bulk_from_producer(std::forward<Args>(args)...) };
			m_count.fetchAndSubOrdered(ret);
			return ret;
		}

		void swap(XConcurrentQueueProxy & other) noexcept {
			m_q.swap(other.m_q);
			auto const v{ m_count.loadRelaxed() };
			m_count.storeRelaxed(other.m_count.loadRelaxed());
			other.m_count.storeRelaxed(v);
		}

		XConcurrentQueueProxy(XConcurrentQueueProxy && other) noexcept
		{ swap(other); }

		XConcurrentQueueProxy& operator=(XConcurrentQueueProxy && other) noexcept
		{ swap(other); return *this; }

		X_DISABLE_COPY(XConcurrentQueueProxy)
	};

	template<typename T,typename Traits,typename QueueType>
	struct XBlockingConcurrentQueueProxy
		: XConcurrentQueueProxy<T,Traits,QueueType>
	{
	private:
		using Base = XConcurrentQueueProxy<T,Traits,QueueType>;

	public:
		using ConcurrentQueue = Base::ConcurrentQueue;
		using value_type = ConcurrentQueue::value_type;
		using size_t = ConcurrentQueue::size_t;
		using index_t = ConcurrentQueue::index_t;
		using producer_token_t = ConcurrentQueue::producer_token_t;
		using consumer_token_t = ConcurrentQueue::consumer_token_t;

		template<typename ...Args>
		constexpr explicit XBlockingConcurrentQueueProxy(Args && ...args)
			: Base{std::forward<Args>(args)...} {}

		template<typename ...Args>
		constexpr void wait_dequeue(Args && ...args) NOEXCEPT_(wait_dequeue) {
			this->m_q.wait_dequeue(std::forward<Args>(args)...);
			this->m_count.deref();
		}

		template<typename ...Args>
		constexpr bool wait_dequeue_timed(Args && ...args) NOEXCEPT_(wait_dequeue_timed) {
			auto const ret { this->m_q.wait_dequeue_timed(std::forward<Args>(args)...) };
			if (ret) { this->m_count.deref(); }
			return ret;
		}

#define MAKE_WAIT_DEQUEUE_BULK_FUNC(wdb_fn) \
		template<typename ...Args> \
		constexpr size_t wdb_fn(Args && ...args) NOEXCEPT_(wdb_fn) { \
			auto const len { this->m_q.wdb_fn(std::forward<Args>(args)...) }; \
			this->m_count.fetchAndSubOrdered(len); \
			return len; \
		}

		MAKE_WAIT_DEQUEUE_BULK_FUNC(wait_dequeue_bulk)
		MAKE_WAIT_DEQUEUE_BULK_FUNC(wait_dequeue_bulk_timed)

#undef MAKE_WAIT_DEQUEUE_BULK_FUNC
#undef CHECK_NOEXCEPT_
#undef NOEXCEPT_

		X_DEFAULT_MOVE(XBlockingConcurrentQueueProxy)
		X_DISABLE_COPY(XBlockingConcurrentQueueProxy)
	};

	template<typename ...Args>
	constexpr void swap(XConcurrentQueueProxy<Args...> & lhs
		,XConcurrentQueueProxy<Args...> & rhs) noexcept
	{ lhs.swap(rhs); }

	template<typename ...Args>
	constexpr void swap(XBlockingConcurrentQueueProxy<Args...> & lhs
		,XBlockingConcurrentQueueProxy<Args...> & rhs) noexcept
	{ lhs.swap(rhs); }

}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
