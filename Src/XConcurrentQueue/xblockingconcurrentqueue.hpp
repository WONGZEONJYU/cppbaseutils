#ifndef XUTILS2_X_BLOCKING_CONCURRENT_QUEUE_HPP
#define XUTILS2_X_BLOCKING_CONCURRENT_QUEUE_HPP 1

// Provides an efficient blocking version of moodycamel::ConcurrentQueue.
// ©2015-2020 Cameron Desrochers. Distributed under the terms of the simplified
// BSD license, available at the top of concurrentqueue.h.
// Also dual-licensed under the Boost Software License (see LICENSE.md)
// Uses Jeff Preshing's semaphore implementation (under the terms of its
// separate zlib license, see lightweightsemaphore.h).

#pragma once

#define XUTILS2_X_BLOCKING_CONCURRENT_QUEUE_ABSTRACT_HPP_
#include <XConcurrentQueue/xblockingconcurrentqueueabstract.hpp>
#undef XUTILS2_X_BLOCKING_CONCURRENT_QUEUE_ABSTRACT_HPP_

#include <chrono>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace moodycamel {

	template<typename,typename = XConcurrentQueueDefaultTraits>
	class XBlockingConcurrentQueue;

	template<typename,typename,typename> struct XBlockingConcurrentQueueProxy;

	// This is a blocking version of the queue. It has an almost identical interface to
	// the normal non-blocking version, with the addition of various wait_dequeue() methods
	// and the removal of producer-specific dequeue methods.
	template<typename T, typename Traits>
	class XBlockingConcurrentQueue
		: public XBlockingConcurrentQueueAbstract<T, Traits>
	{
		using Base = XBlockingConcurrentQueueAbstract<T, Traits>;

	public:
		using producer_token_t = Base::producer_token_t;
		using consumer_token_t = Base::consumer_token_t;
		using value_type = Base::value_type;
		using index_t = Base::index_t;
		using size_t = Base::size_t;
		using ssize_t = Base::ssize_t;

	protected:
		// Creates a queue with at least `capacity` element slots; note that the
		// actual number of elements that can be inserted without additional memory
		// allocation depends on the number of producers and the block size (e.g. if
		// the block size is equal to `capacity`, only a single block will be allocated
		// up-front, which means only a single producer will be able to enqueue elements
		// without an extra allocation -- blocks aren't shared between producers).
		// This method is not thread safe -- it is up to the user to ensure that the
		// queue is fully constructed before it starts being used by other threads (this
		// includes making the memory effects of construction visible, possibly with a
		// memory barrier).
		explicit XBlockingConcurrentQueue(size_t const capacity = 6 * Base::BLOCK_SIZE)
			: Base { capacity } {}

		XBlockingConcurrentQueue(size_t const minCapacity, size_t const maxExplicitProducers, size_t const maxImplicitProducers)
			: Base { minCapacity, maxExplicitProducers, maxImplicitProducers } {}

	public:
		// Disable copying and copy assignment
		X_DISABLE_COPY(XBlockingConcurrentQueue)

		// Moving is supported, but note that it is *not* a thread-safe operation.
		// Nobody can use the queue while it's being moved, and the memory effects
		// of that move must be propagated to other threads before they can use it.
		// Note: When a queue is moved, its tokens are still valid but can only be
		// used with the destination queue (i.e. semantically they are moved along
		// with the queue itself).
		X_DEFAULT_MOVE(XBlockingConcurrentQueue)

		// Enqueues a single item (by copying it).
		// Allocates memory if required. Only fails if memory allocation fails (or implicit
		// production is disabled because Traits::INITIAL_IMPLICIT_PRODUCER_HASH_SIZE is 0,
		// or Traits::MAX_SUBQUEUE_SIZE has been defined and would be surpassed).
		// Thread-safe.
		constexpr bool enqueue(value_type const & item) {
			if ((details::likely)(this->m_inner_.enqueue(item))) {
				this->m_sema_->signal();
				return true;
			}
			return {};
		}

		// Enqueues a single item (by moving it, if possible).
		// Allocates memory if required. Only fails if memory allocation fails (or implicit
		// production is disabled because Traits::INITIAL_IMPLICIT_PRODUCER_HASH_SIZE is 0,
		// or Traits::MAX_SUBQUEUE_SIZE has been defined and would be surpassed).
		// Thread-safe.
		constexpr bool enqueue(value_type && item) {
			if ((details::likely)(this->m_inner_.enqueue(std::move(item)))) {
				this->m_sema_->signal();
				return true;
			}
			return {};
		}

		// Enqueues a single item (by copying it) using an explicit producer token.
		// Allocates memory if required. Only fails if memory allocation fails (or
		// Traits::MAX_SUBQUEUE_SIZE has been defined and would be surpassed).
		// Thread-safe.
		constexpr bool enqueue(producer_token_t const & token, value_type const & item) {
			if ((details::likely)(this->m_inner_.enqueue(token, item))) {
				this->m_sema_->signal();
				return true;
			}
			return {};
		}

		// Enqueues a single item (by moving it, if possible) using an explicit producer token.
		// Allocates memory if required. Only fails if memory allocation fails (or
		// Traits::MAX_SUBQUEUE_SIZE has been defined and would be surpassed).
		// Thread-safe.
		constexpr bool enqueue(producer_token_t const & token, value_type && item) {
			if ((details::likely)(this->m_inner_.enqueue(token, std::move(item)))) {
				this->m_sema_->signal();
				return true;
			}
			return {};
		}

		// Enqueues several items.
		// Allocates memory if required. Only fails if memory allocation fails (or
		// implicit production is disabled because Traits::INITIAL_IMPLICIT_PRODUCER_HASH_SIZE
		// is 0, or Traits::MAX_SUBQUEUE_SIZE has been defined and would be surpassed).
		// Note: Use std::make_move_iterator if the elements should be moved instead of copied.
		// Thread-safe.
		template<typename It>
		constexpr bool enqueue_bulk(It && itemFirst, size_t const count) {
			if ((details::likely)(this->m_inner_.enqueue_bulk(std::forward<decltype(itemFirst)>(itemFirst), count))) {
				this->m_sema_->signal(static_cast<Base::LightweightSemaphore::ssize_t>(static_cast<Base::ssize_t>(count)));
				return true;
			}
			return {};
		}

		// Enqueues several items using an explicit producer token.
		// Allocates memory if required. Only fails if memory allocation fails
		// (or Traits::MAX_SUBQUEUE_SIZE has been defined and would be surpassed).
		// Note: Use std::make_move_iterator if the elements should be moved
		// instead of copied.
		// Thread-safe.
		template<typename It>
		constexpr bool enqueue_bulk(producer_token_t const & token, It && itemFirst, size_t const count) {
			if ((details::likely)(this->m_inner_.enqueue_bulk(token, std::forward<decltype(itemFirst)>(itemFirst), count))) {
				this->m_sema_->signal(static_cast<Base::LightweightSemaphore::ssize_t>(static_cast<Base::ssize_t>(count)));
				return true;
			}
			return {};
		}

		// Enqueues a single item (by copying it).
		// Does not allocate memory. Fails if not enough room to enqueue (or implicit
		// production is disabled because Traits::INITIAL_IMPLICIT_PRODUCER_HASH_SIZE
		// is 0).
		// Thread-safe.
		constexpr bool try_enqueue(value_type const & item){
			if (this->m_inner_.try_enqueue(item)) {
				this->m_sema_->signal();
				return true;
			}
			return {};
		}

		// Enqueues a single item (by moving it, if possible).
		// Does not allocate memory (except for one-time implicit producer).
		// Fails if not enough room to enqueue (or implicit production is
		// disabled because Traits::INITIAL_IMPLICIT_PRODUCER_HASH_SIZE is 0).
		// Thread-safe.
		constexpr bool try_enqueue(value_type && item) {
			if (this->m_inner_.try_enqueue(std::move(item))) {
				this->m_sema_->signal();
				return true;
			}
			return {};
		}

		// Enqueues a single item (by copying it) using an explicit producer token.
		// Does not allocate memory. Fails if not enough room to enqueue.
		// Thread-safe.
		constexpr bool try_enqueue(producer_token_t const & token, value_type const & item) {
			if (this->m_inner_.try_enqueue(token, item)) {
				this->m_sema_->signal();
				return true;
			}
			return {};
		}

		// Enqueues a single item (by moving it, if possible) using an explicit producer token.
		// Does not allocate memory. Fails if not enough room to enqueue.
		// Thread-safe.
		constexpr bool try_enqueue(producer_token_t const & token, value_type && item) {
			if (this->m_inner_.try_enqueue(token,std::move(item))) {
				this->m_sema_->signal();
				return true;
			}
			return {};
		}

		// Enqueues several items.
		// Does not allocate memory (except for one-time implicit producer).
		// Fails if not enough room to enqueue (or implicit production is
		// disabled because Traits::INITIAL_IMPLICIT_PRODUCER_HASH_SIZE is 0).
		// Note: Use std::make_move_iterator if the elements should be moved
		// instead of copied.
		// Thread-safe.
		template<typename It>
		constexpr bool try_enqueue_bulk(It && itemFirst, size_t const count) {
			if (this->m_inner_.try_enqueue_bulk(std::forward<decltype(itemFirst)>(itemFirst), count)) {
				this->m_sema_->signal(static_cast<Base::LightweightSemaphore::ssize_t>(static_cast<Base::ssize_t>(count)));
				return true;
			}
			return {};
		}

		// Enqueues several items using an explicit producer token.
		// Does not allocate memory. Fails if not enough room to enqueue.
		// Note: Use std::make_move_iterator if the elements should be moved
		// instead of copied.
		// Thread-safe.
		template<typename It>
		constexpr bool try_enqueue_bulk(producer_token_t const & token, It && itemFirst, size_t const count) {
			if (this->m_inner_.try_enqueue_bulk(token, std::forward<decltype(itemFirst)>(itemFirst), count)) {
				this->m_sema_->signal(static_cast<Base::LightweightSemaphore::ssize_t>(static_cast<Base::ssize_t>(count)));
				return true;
			}
			return {};
		}

		// Attempts to dequeue from the queue.
		// Returns false if all producer streams appeared empty at the time they
		// were checked (so, the queue is likely but not guaranteed to be empty).
		// Never allocates. Thread-safe.
		template<typename U>
		constexpr bool try_dequeue(U & item) {
			if (this->m_sema_->tryWait())
			{ while (!this->m_inner_.try_dequeue(item)) {} return true; }
			return {};
		}

		// Attempts to dequeue from the queue using an explicit consumer token.
		// Returns false if all producer streams appeared empty at the time they
		// were checked (so, the queue is likely but not guaranteed to be empty).
		// Never allocates. Thread-safe.
		template<typename U>
		constexpr bool try_dequeue(consumer_token_t & token, U & item) {
			if (this->m_sema_->tryWait())
			{ while (!this->m_inner_.try_dequeue(token, item)) {} return true; }
			return {};
		}

		// Attempts to dequeue several elements from the queue.
		// Returns the number of items actually dequeued.
		// Returns 0 if all producer streams appeared empty at the time they
		// were checked (so, the queue is likely but not guaranteed to be empty).
		// Never allocates. Thread-safe.
		template<typename It>
		constexpr size_t try_dequeue_bulk(It && itemFirst, size_t max) {
			size_t count {};
			max = static_cast<size_t>(this->m_sema_->tryWaitMany(static_cast<Base::LightweightSemaphore::ssize_t>(static_cast<Base::ssize_t>(max))));
			while (count != max)
			{ count += this->m_inner_.try_dequeue_bulk(std::forward<decltype(itemFirst)>(itemFirst), max - count); }
			return count;
		}

		// Attempts to dequeue several elements from the queue using an explicit consumer token.
		// Returns the number of items actually dequeued.
		// Returns 0 if all producer streams appeared empty at the time they
		// were checked (so, the queue is likely but not guaranteed to be empty).
		// Never allocates. Thread-safe.
		template<typename It>
		constexpr size_t try_dequeue_bulk(consumer_token_t & token, It && itemFirst, size_t max) {
			size_t count {};
			max = static_cast<size_t>(this->m_sema_->tryWaitMany(static_cast<Base::LightweightSemaphore::ssize_t>(static_cast<Base::ssize_t>(max))));
			while (count != max) { count += this->m_inner_.try_dequeue_bulk(token, std::forward<decltype(itemFirst)>(itemFirst), max - count); }
			return count;
		}

		// Blocks the current thread until there's something to dequeue, then
		// dequeues it.
		// Never allocates. Thread-safe.
		template<typename U>
		constexpr void wait_dequeue(U & item)
		{ while (!this->m_sema_->wait()) {} while (!this->m_inner_.try_dequeue(item)) {} }

		// Blocks the current thread until either there's something to dequeue
		// or the timeout (specified in microseconds) expires. Returns false
		// without setting `item` if the timeout expires, otherwise assigns
		// to `item` and returns true.
		// Using a negative timeout indicates an indefinite timeout,
		// and is thus functionally equivalent to calling wait_dequeue.
		// Never allocates. Thread-safe.
		template<typename U>
		constexpr bool wait_dequeue_timed(U & item, std::int64_t const timeout_usecs) {
			if (!this->m_sema_->wait(timeout_usecs)) { return {}; }
			while (!this->m_inner_.try_dequeue(item)) {}
			return true;
		}

		// Blocks the current thread until either there's something to dequeue
		// or the timeout expires. Returns false without setting `item` if the
		// timeout expires, otherwise assigns to `item` and returns true.
		// Never allocates. Thread-safe.
		template<typename U, typename Rep, typename Period>
		constexpr bool wait_dequeue_timed(U & item, std::chrono::duration<Rep, Period> const & timeout)
		{ return wait_dequeue_timed(item, std::chrono::duration_cast<std::chrono::microseconds>(timeout).count()); }

		// Blocks the current thread until there's something to dequeue, then
		// dequeues it using an explicit consumer token.
		// Never allocates. Thread-safe.
		template<typename U>
		constexpr void wait_dequeue(consumer_token_t & token, U & item)
		{ while (!this->m_sema_->wait()) {} while (!this->m_inner_.try_dequeue(token, item)) {} }

		// Blocks the current thread until either there's something to dequeue
		// or the timeout (specified in microseconds) expires. Returns false
		// without setting `item` if the timeout expires, otherwise assigns
		// to `item` and returns true.
		// Using a negative timeout indicates an indefinite timeout,
		// and is thus functionally equivalent to calling wait_dequeue.
		// Never allocates. Thread-safe.
		template<typename U>
		constexpr bool wait_dequeue_timed(consumer_token_t & token, U & item, std::int64_t const timeout_usecs) {
			if (!this->m_sema_->wait(timeout_usecs)) { return {}; }
			while (!this->m_inner_.try_dequeue(token, item)) {}
			return true;
		}

		// Blocks the current thread until either there's something to dequeue
		// or the timeout expires. Returns false without setting `item` if the
		// timeout expires, otherwise assigns to `item` and returns true.
		// Never allocates. Thread-safe.
		template<typename U, typename Rep, typename Period>
		constexpr bool wait_dequeue_timed(consumer_token_t & token, U & item, std::chrono::duration<Rep, Period> const & timeout)
		{ return wait_dequeue_timed(token, item, std::chrono::duration_cast<std::chrono::microseconds>(timeout).count()); }

		// Attempts to dequeue several elements from the queue.
		// Returns the number of items actually dequeued, which will
		// always be at least one (this method blocks until the queue
		// is non-empty) and at most max.
		// Never allocates. Thread-safe.
		template<typename It>
		constexpr size_t wait_dequeue_bulk(It && itemFirst, size_t max) {
			size_t count {};
			max = static_cast<size_t>(this->m_sema_->waitMany(static_cast<Base::LightweightSemaphore::ssize_t>(static_cast<Base::ssize_t>(max))));
			while (count != max) { count += this->m_inner_.try_dequeue_bulk(std::forward<decltype(itemFirst)>(itemFirst), max - count); }
			return count;
		}

		// Attempts to dequeue several elements from the queue.
		// Returns the number of items actually dequeued, which can
		// be 0 if the timeout expires while waiting for elements,
		// and at most max.
		// Using a negative timeout indicates an indefinite timeout,
		// and is thus functionally equivalent to calling wait_dequeue_bulk.
		// Never allocates. Thread-safe.
		template<typename It>
		constexpr size_t wait_dequeue_bulk_timed(It && itemFirst, size_t max, std::int64_t const timeout_usecs) {
			size_t count{};
			max = static_cast<size_t>(this->m_sema_->waitMany(static_cast<Base::LightweightSemaphore::ssize_t>(static_cast<Base::ssize_t>(max)), timeout_usecs));
			while (count != max) { count += this->m_inner_.try_dequeue_bulk(std::forward<decltype(itemFirst)>(itemFirst), max - count); }
			return count;
		}

		// Attempts to dequeue several elements from the queue.
		// Returns the number of items actually dequeued, which can
		// be 0 if the timeout expires while waiting for elements,
		// and at most max.
		// Never allocates. Thread-safe.
		template<typename It, typename Rep, typename Period>
		constexpr size_t wait_dequeue_bulk_timed(It && itemFirst, size_t const max, std::chrono::duration<Rep, Period> const & timeout)
		{ return wait_dequeue_bulk_timed(std::forward<decltype(itemFirst)>(itemFirst), max,std::chrono::duration_cast<std::chrono::microseconds>(timeout).count()); }

		// Attempts to dequeue several elements from the queue using an explicit consumer token.
		// Returns the number of items actually dequeued, which will
		// always be at least one (this method blocks until the queue
		// is non-empty) and at most max.
		// Never allocates. Thread-safe.
		template<typename It>
		constexpr size_t wait_dequeue_bulk(consumer_token_t & token, It && itemFirst, size_t max) {
			size_t count {};
			max = static_cast<size_t>(this->m_sema_->waitMany(static_cast<Base::LightweightSemaphore::ssize_t>(static_cast<Base::ssize_t>(max))));
			while (count != max) { count += this->m_inner_.try_dequeue_bulk(token, std::forward<decltype(itemFirst)>(itemFirst), max - count); }
			return count;
		}

		// Attempts to dequeue several elements from the queue using an explicit consumer token.
		// Returns the number of items actually dequeued, which can
		// be 0 if the timeout expires while waiting for elements,
		// and at most max.
		// Using a negative timeout indicates an indefinite timeout,
		// and is thus functionally equivalent to calling wait_dequeue_bulk.
		// Never allocates. Thread-safe.
		template<typename It>
		constexpr size_t wait_dequeue_bulk_timed(consumer_token_t & token, It && itemFirst, size_t max, std::int64_t const timeout_usecs) {
			size_t count {};
			max = static_cast<size_t>(this->m_sema_->waitMany(static_cast<Base::LightweightSemaphore::ssize_t>(static_cast<Base::ssize_t>(max)), timeout_usecs));
			while (count != max) { count += this->m_inner_.try_dequeue_bulk(token, std::forward<decltype(itemFirst)>(itemFirst), max - count); }
			return count;
		}

		// Attempts to dequeue several elements from the queue using an explicit consumer token.
		// Returns the number of items actually dequeued, which can
		// be 0 if the timeout expires while waiting for elements,
		// and at most max.
		// Never allocates. Thread-safe.
		template<typename It, typename Rep, typename Period>
		constexpr size_t wait_dequeue_bulk_timed(consumer_token_t & token, It && itemFirst, size_t const max, std::chrono::duration<Rep, Period> const & timeout) {
			return wait_dequeue_bulk_timed<It&>(token,std::forward<decltype(itemFirst)>(itemFirst)
					,max,std::chrono::duration_cast<std::chrono::microseconds>(timeout).count());
		}

		// Returns an estimate of the total number of elements currently in the queue. This
		// estimate is only accurate if the queue has completely stabilized before it is called
		// (i.e. all enqueue and dequeue operations have completed and their memory effects are
		// visible on the calling thread, and no further operations start while this method is
		// being called).
		// Thread-safe.
		[[nodiscard]] constexpr size_t size_approx() const noexcept
		{ return static_cast<size_t>(this->m_sema_->availableApprox()); }

		[[nodiscard]] constexpr bool empty() const noexcept { return !size_approx(); }
		[[nodiscard]] constexpr bool ioEmpty() const noexcept { return !size_approx(); }

		// Returns true if the underlying atomic variables used by
		// the queue are lock-free (they should be on most platforms).
		// Thread-safe.
		static constexpr bool is_lock_free() noexcept
		{ return Base::XConcurrentQueue::is_lock_free(); }

		template<typename ,typename >
		friend class XBlockingConcurrentQueueAbstract;

		template<typename,typename,typename> friend struct XBlockingConcurrentQueueProxy;
		template<typename,typename,typename> friend struct XConcurrentQueueProxy;

	};

	template<typename T, typename Traits>
	static constexpr void swap(XBlockingConcurrentQueue<T, Traits> & a, XBlockingConcurrentQueue<T, Traits> & b) noexcept
	{ a.swap(b); }

}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
