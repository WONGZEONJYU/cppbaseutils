#ifndef XUTILS2_X_CONCURRENT_QUEUE_HPP
#define XUTILS2_X_CONCURRENT_QUEUE_HPP 1

// Provides a C++11 implementation of a multi-producer, multi-consumer lock-free queue.
// An overview, including benchmark results, is provided here:
//     http://moodycamel.com/blog/2014/a-fast-general-purpose-lock-free-queue-for-c++
// The full design is also described in excruciating detail at:
//    http://moodycamel.com/blog/2014/detailed-design-of-a-lock-free-queue

// Simplified BSD license:
// Copyright (c) 2013-2020, Cameron Desrochers.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// - Redistributions of source code must retain the above copyright notice, this list of
// conditions and the following disclaimer.
// - Redistributions in binary form must reproduce the above copyright notice, this list of
// conditions and the following disclaimer in the documentation and/or other materials
// provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
// TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Also dual-licensed under the Boost Software License (see LICENSE.md)

#pragma once

#define XUTILS2_X_CONCURRENT_QUEUE_ABSTRACT_HPP_
#include <XConcurrentQueue/xconcurrentqueueabstract.hpp>
#undef XUTILS2_X_CONCURRENT_QUEUE_ABSTRACT_HPP_

#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
// Disable -Wconversion warnings (spuriously triggered when Traits::size_t and
// Traits::index_t are set to < 32 bits, causing integer promotion, causing warnings
// upon assigning any computed values)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"

#ifdef MCDBGQ_USE_RELACY
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#endif
#endif

#if defined(_MSC_VER) && (!defined(_HAS_CXX17) || !_HAS_CXX17)
// VS2019 with /W4 warns about constant conditional expressions but unless /std=c++17 or higher
// does not support `if constexpr`, so we have no choice but to simply disable the warning
#pragma warning(push)
#pragma warning(disable: 4127)  // conditional expression is constant
#endif

#ifdef MOODYCAMEL_QUEUE_INTERNAL_DEBUG
#include "concurrentqueue_internal_debug_p.hpp"
#endif

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace moodycamel {

	template<typename,typename = XConcurrentQueueDefaultTraits>
	class XConcurrentQueue;

	template<typename T, typename Traits>
	class XConcurrentQueue : public XConcurrentQueueAbstract<T,Traits> {
		using Base = XConcurrentQueueAbstract<T,Traits>;
	public:
		using value_type = Base::value_type;

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
		explicit XConcurrentQueue(size_t const capacity = 32 * Base::BLOCK_SIZE) {
			this->implicitProducerHashResizeInProgress.clear(std::memory_order_relaxed);
			this->populate_initial_implicit_producer_hash();
			this->populate_initial_block_list(capacity / Base::BLOCK_SIZE + ( capacity & Base::BLOCK_SIZE - 1 ? 1 : 0) );

#ifdef MOODYCAMEL_QUEUE_INTERNAL_DEBUG
			// Track all the producers using a fully-resolved typed list for
			// each kind; this makes it possible to debug them starting from
			// the root queue object (otherwise wacky casts are needed that
			// don't compile in the debugger's expression evaluator).
			explicitProducers.storeRelaxed({});
			implicitProducers.storeRelaxed({});
#endif
		}

		// Computes the correct amount of pre-allocated blocks for you based
		// on the minimum number of elements you want available at any given
		// time, and the maximum concurrent number of each type of producer.
		XConcurrentQueue(size_t const minCapacity, size_t const maxExplicitProducers, size_t const maxImplicitProducers) {
			this->implicitProducerHashResizeInProgress.clear(std::memory_order_relaxed);
			this->populate_initial_implicit_producer_hash();
			auto const blocks{
				((minCapacity + Base::BLOCK_SIZE - 1) / Base::BLOCK_SIZE - 1) * (maxExplicitProducers + 1) + 2 * (maxExplicitProducers + maxImplicitProducers)
			};
			populate_initial_block_list(blocks);
#ifdef MOODYCAMEL_QUEUE_INTERNAL_DEBUG
			explicitProducers.storeRelaxed({});
			implicitProducers.storeRelaxed({});
#endif
		}

		// Note: The queue should not be accessed concurrently while it's
		// being deleted. It's up to the user to synchronize this.
		// This method is not thread safe.
		~XConcurrentQueue() override = default;

		// Disable copying and copy assignment
		X_DISABLE_COPY(XConcurrentQueue)

		// Moving is supported, but note that it is *not* a thread-safe operation.
		// Nobody can use the queue while it's being moved, and the memory effects
		// of that move must be propagated to other threads before they can use it.
		// Note: When a queue is moved, its tokens are still valid but can only be
		// used with the destination queue (i.e. semantically they are moved along
		// with the queue itself).
		XConcurrentQueue(XConcurrentQueue && other) MOODYCAMEL_NOEXCEPT = default;
		XConcurrentQueue& operator=(XConcurrentQueue && other) MOODYCAMEL_NOEXCEPT = default;

		// Enqueues a single item (by copying it).
		// Allocates memory if required. Only fails if memory allocation fails (or implicit
		// production is disabled because Traits::INITIAL_IMPLICIT_PRODUCER_HASH_SIZE is 0,
		// or Traits::MAX_SUBQUEUE_SIZE has been defined and would be surpassed).
		// Thread-safe.
		constexpr bool enqueue(value_type const & item) {
			MOODYCAMEL_CONSTEXPR_IF (!Base::INITIAL_IMPLICIT_PRODUCER_HASH_SIZE ) { return {}; }
			else { return this->inner_enqueue<Base::CanAlloc>(item); }
		}

		// Enqueues a single item (by moving it, if possible).
		// Allocates memory if required. Only fails if memory allocation fails (or implicit
		// production is disabled because Traits::INITIAL_IMPLICIT_PRODUCER_HASH_SIZE is 0,
		// or Traits::MAX_SUBQUEUE_SIZE has been defined and would be surpassed).
		// Thread-safe.
		constexpr bool enqueue(value_type && item) {
			MOODYCAMEL_CONSTEXPR_IF (!Base::INITIAL_IMPLICIT_PRODUCER_HASH_SIZE ) { return {}; }
			else { return this->inner_enqueue<Base::CanAlloc>(std::move(item)); }
		}

		// Enqueues a single item (by copying it) using an explicit producer token.
		// Allocates memory if required. Only fails if memory allocation fails (or
		// Traits::MAX_SUBQUEUE_SIZE has been defined and would be surpassed).
		// Thread-safe.
		constexpr bool enqueue(Base::producer_token_t const & token, value_type const & item)
		{ return this->inner_enqueue<Base::CanAlloc>(token, item); }

		// Enqueues a single item (by moving it, if possible) using an explicit producer token.
		// Allocates memory if required. Only fails if memory allocation fails (or
		// Traits::MAX_SUBQUEUE_SIZE has been defined and would be surpassed).
		// Thread-safe.
		constexpr bool enqueue(Base::producer_token_t const & token, T && item)
		{ return this->inner_enqueue<Base::CanAlloc>(token, std::move(item)); }

		// Enqueues several items.
		// Allocates memory if required. Only fails if memory allocation fails (or
		// implicit production is disabled because Traits::INITIAL_IMPLICIT_PRODUCER_HASH_SIZE
		// is 0, or Traits::MAX_SUBQUEUE_SIZE has been defined and would be surpassed).
		// Note: Use std::make_move_iterator if the elements should be moved instead of copied.
		// Thread-safe.
		template<typename It>
		constexpr bool enqueue_bulk(It && itemFirst, size_t const count) {
			MOODYCAMEL_CONSTEXPR_IF (!Base::INITIAL_IMPLICIT_PRODUCER_HASH_SIZE ) { return {}; }
			else { return this->inner_enqueue_bulk<Base::CanAlloc>(std::forward<decltype(itemFirst)>(itemFirst), count); }
		}

		// Enqueues several items using an explicit producer token.
		// Allocates memory if required. Only fails if memory allocation fails
		// (or Traits::MAX_SUBQUEUE_SIZE has been defined and would be surpassed).
		// Note: Use std::make_move_iterator if the elements should be moved
		// instead of copied.
		// Thread-safe.
		template<typename It>
		constexpr bool enqueue_bulk(Base::producer_token_t const & token, It && itemFirst, size_t const count)
		{ return this->inner_enqueue_bulk<Base::CanAlloc>(token, std::forward<decltype(itemFirst)>(itemFirst), count); }

		// Enqueues a single item (by copying it).
		// Does not allocate memory. Fails if not enough room to enqueue (or implicit
		// production is disabled because Traits::INITIAL_IMPLICIT_PRODUCER_HASH_SIZE
		// is 0).
		// Thread-safe.
		constexpr bool try_enqueue(value_type const & item) {
			MOODYCAMEL_CONSTEXPR_IF (!Base::INITIAL_IMPLICIT_PRODUCER_HASH_SIZE) { return {}; }
			else { return this->inner_enqueue<Base::CannotAlloc>(item); }
		}

		// Enqueues a single item (by moving it, if possible).
		// Does not allocate memory (except for one-time implicit producer).
		// Fails if not enough room to enqueue (or implicit production is
		// disabled because Traits::INITIAL_IMPLICIT_PRODUCER_HASH_SIZE is 0).
		// Thread-safe.
		constexpr bool try_enqueue(value_type && item) {
			MOODYCAMEL_CONSTEXPR_IF (!Base::INITIAL_IMPLICIT_PRODUCER_HASH_SIZE ) { return {}; }
			else { return this->inner_enqueue<Base::CannotAlloc>(std::move(item)); }
		}

		// Enqueues a single item (by copying it) using an explicit producer token.
		// Does not allocate memory. Fails if not enough room to enqueue.
		// Thread-safe.
		constexpr bool try_enqueue(Base::producer_token_t const & token, value_type const & item)
		{ return this->inner_enqueue<Base::CannotAlloc>(token, item); }

		// Enqueues a single item (by moving it, if possible) using an explicit producer token.
		// Does not allocate memory. Fails if not enough room to enqueue.
		// Thread-safe.
		constexpr bool try_enqueue(Base::producer_token_t const & token, value_type && item)
		{ return this->inner_enqueue<Base::CannotAlloc>(token, std::move(item)); }

		// Enqueues several items.
		// Does not allocate memory (except for one-time implicit producer).
		// Fails if not enough room to enqueue (or implicit production is
		// disabled because Traits::INITIAL_IMPLICIT_PRODUCER_HASH_SIZE is 0).
		// Note: Use std::make_move_iterator if the elements should be moved
		// instead of copied.
		// Thread-safe.
		template<typename It>
		constexpr bool try_enqueue_bulk(It && itemFirst, size_t const count) {
			MOODYCAMEL_CONSTEXPR_IF (!Base::INITIAL_IMPLICIT_PRODUCER_HASH_SIZE ) { return {}; }
			else { return this-> template inner_enqueue_bulk<Base::CannotAlloc>(std::forward<decltype(itemFirst)>(itemFirst), count); }
		}

		// Enqueues several items using an explicit producer token.
		// Does not allocate memory. Fails if not enough room to enqueue.
		// Note: Use std::make_move_iterator if the elements should be moved
		// instead of copied.
		// Thread-safe.
		template<typename It>
		constexpr bool try_enqueue_bulk(Base::producer_token_t const & token, It && itemFirst, size_t const count)
		{ return this->template inner_enqueue_bulk<Base::CannotAlloc>(token, std::forward<decltype(itemFirst)>(itemFirst), count); }

		// Attempts to dequeue from the queue.
		// Returns false if all producer streams appeared empty at the time they
		// were checked (so, the queue is likely but not guaranteed to be empty).
		// Never allocates. Thread-safe.
		template<typename U>
		constexpr bool try_dequeue(U & item) {
			// Instead of simply trying each producer in turn (which could cause needless contention on the first
			// producer), we score them heuristically.
			size_t nonEmptyCount {},bestSize {};
			typename Base::ProducerBase * best {};
			for (auto ptr{ this->producerListTail.loadAcquire()};
				nonEmptyCount < 3 && ptr; ptr = ptr->next_prod())
			{
				if (auto const size{ptr->size_approx()};size > 0) {
					if (size > bestSize) {
						bestSize = size;
						best = ptr;
					}
					++nonEmptyCount;
				}
			}

			// If there was at least one non-empty queue but it appears empty at the time
			// we try to dequeue from it, we need to make sure every queue's been tried
			if (nonEmptyCount > 0) {
				if ((details::likely)(best->dequeue(item))) { return true; }
				for (auto ptr { this->producerListTail.loadAcquire() };
					ptr; ptr = ptr->next_prod())
				{ if (ptr != best && ptr->dequeue(item)) { return true; } }
			}
			return {};
		}

		// Attempts to dequeue from the queue.
		// Returns false if all producer streams appeared empty at the time they
		// were checked (so, the queue is likely but not guaranteed to be empty).
		// This differs from the try_dequeue(item) method in that this one does
		// not attempt to reduce contention by interleaving the order that producer
		// streams are dequeued from. So, using this method can reduce overall throughput
		// under contention, but will give more predictable results in single-threaded
		// consumer scenarios. This is mostly only useful for internal unit tests.
		// Never allocates. Thread-safe.
		template<typename U>
		constexpr bool try_dequeue_non_interleaved(U & item) {
			for (auto ptr{ this->producerListTail.loadAcquire() };
				ptr ; ptr = ptr->next_prod())
			{ if (ptr->dequeue(item)) { return true; } }
			return {};
		}

		// Attempts to dequeue from the queue using an explicit consumer token.
		// Returns false if all producer streams appeared empty at the time they
		// were checked (so, the queue is likely but not guaranteed to be empty).
		// Never allocates. Thread-safe.
		template<typename U>
		constexpr bool try_dequeue(Base::consumer_token_t & token, U & item) {
			// The idea is roughly as follows:
			// Every 256 items from one producer, make everyone rotate (increase the global offset) -> this means the highest efficiency consumer dictates the rotation speed of everyone else, more or less
			// If you see that the global offset has changed, you must reset your consumption counter and move to your designated place
			// If there's no items where you're supposed to be, keep moving until you find a producer with some items
			// If the global offset has not changed but you've run out of items to consume, move over from your current position until you find an producer with something in it

			if (!token.desiredProducer || token.lastKnownGlobalOffset != this->globalExplicitConsumerOffset.loadRelaxed() )
			{ if (!this->update_current_producer_after_rotation(token)) { return {}; } }

			// If there was at least one non-empty queue but it appears empty at the time
			// we try to dequeue from it, we need to make sure every queue's been tried
			if (static_cast< Base::ProducerBase *>(token.currentProducer)->dequeue(item)) {
				if (++token.itemsConsumedFromCurrent == Base::EXPLICIT_CONSUMER_CONSUMPTION_QUOTA_BEFORE_ROTATE)
				{ this->globalExplicitConsumerOffset.fetchAndAddRelaxed(1); }
				return true;
			}

			auto const tail{ this->producerListTail.loadAcquire() };
			auto ptr{ static_cast<Base::ProducerBase *>(token.currentProducer)->next_prod() };
			if (!ptr) { ptr = tail; }
			while (ptr != static_cast<Base::ProducerBase *>(token.currentProducer)) {
				if (ptr->dequeue(item)) {
					token.currentProducer = ptr;
					token.itemsConsumedFromCurrent = 1;
					return true;
				}
				ptr = ptr->next_prod();
				if (!ptr) { ptr = tail; }
			}
			return {};
		}

		// Attempts to dequeue several elements from the queue.
		// Returns the number of items actually dequeued.
		// Returns 0 if all producer streams appeared empty at the time they
		// were checked (so, the queue is likely but not guaranteed to be empty).
		// Never allocates. Thread-safe.
		template<typename It>
		constexpr size_t try_dequeue_bulk(It && itemFirst, size_t const max) {
			size_t count {};
			for (auto ptr{ this->producerListTail.loadAcquire() };
				ptr; ptr = ptr->next_prod())
			{
				count += ptr->dequeue_bulk(std::forward<decltype(itemFirst)>(itemFirst), max - count);
				if (count == max) { break; }
			}
			return count;
		}

		// Attempts to dequeue several elements from the queue using an explicit consumer token.
		// Returns the number of items actually dequeued.
		// Returns 0 if all producer streams appeared empty at the time they
		// were checked (so, the queue is likely but not guaranteed to be empty).
		// Never allocates. Thread-safe.
		template<typename It>
		constexpr size_t try_dequeue_bulk(Base::consumer_token_t & token, It && itemFirst, size_t const max) {

			if (!token.desiredProducer || token.lastKnownGlobalOffset != this->globalExplicitConsumerOffset.loadRelaxed())
			{ if (!this->update_current_producer_after_rotation(token)) { return {}; } }

			auto count{ static_cast<Base::ProducerBase*>(token.currentProducer)->dequeue_bulk(std::forward<decltype(itemFirst)>(itemFirst), max) };

			if (max == count) {
				if ((token.itemsConsumedFromCurrent += static_cast<std::uint32_t>(max)) >= Base::EXPLICIT_CONSUMER_CONSUMPTION_QUOTA_BEFORE_ROTATE)
				{ Base::globalExplicitConsumerOffset.fetchAndAddRelaxed(1); }
				return max;
			}

			token.itemsConsumedFromCurrent += static_cast<std::uint32_t>(count);
			max -= count;

			auto const tail{ this->producerListTail.loadAcquire() };
			auto ptr{ static_cast<Base::ProducerBase*>(token.currentProducer)->next_prod() };
			if (!ptr) { ptr = tail; }

			while (ptr != static_cast<Base::ProducerBase*>(token.currentProducer)) {
				auto const dequeued{ ptr->dequeue_bulk(std::forward<decltype(itemFirst)>(itemFirst), max)};
				count += dequeued;
				if (dequeued) {
					token.currentProducer = ptr;
					token.itemsConsumedFromCurrent = static_cast<std::uint32_t>(dequeued);
				}

				if (dequeued == max) { break; }
				max -= dequeued;
				ptr = ptr->next_prod();
				if (!ptr) { ptr = tail; }
			}
			return count;
		}

		// Attempts to dequeue from a specific producer's inner queue.
		// If you happen to know which producer you want to dequeue from, this
		// is significantly faster than using the general-case try_dequeue methods.
		// Returns false if the producer's queue appeared empty at the time it
		// was checked (so, the queue is likely but not guaranteed to be empty).
		// Never allocates. Thread-safe.
		template<typename U>
		static constexpr bool try_dequeue_from_producer(Base::producer_token_t const & producer, U & item)
		{ return static_cast<Base::ExplicitProducer*>(producer.producer)->dequeue(item); }

		// Attempts to dequeue several elements from a specific producer's inner queue.
		// Returns the number of items actually dequeued.
		// If you happen to know which producer you want to dequeue from, this
		// is significantly faster than using the general-case try_dequeue methods.
		// Returns 0 if the producer's queue appeared empty at the time it
		// was checked (so, the queue is likely but not guaranteed to be empty).
		// Never allocates. Thread-safe.
		template<typename It>
		static constexpr size_t try_dequeue_bulk_from_producer(Base::producer_token_t const & producer, It && itemFirst, size_t const max)
		{ return static_cast<Base::ExplicitProducer*>(producer.producer)->dequeue_bulk(std::forward<decltype(itemFirst)>(itemFirst), max); }

		// Returns an estimate of the total number of elements currently in the queue. This
		// estimate is only accurate if the queue has completely stabilized before it is called
		// (i.e. all enqueue and dequeue operations have completed and their memory effects are
		// visible on the calling thread, and no further operations start while this method is
		// being called).
		// Thread-safe.
		[[nodiscard]] constexpr size_t size_approx() const {
			size_t size {};
			for (auto ptr{ this->producerListTail.loadAcquire() };
				ptr; ptr = ptr->next_prod())
			{ size += ptr->size_approx(); }
			return size;
		}

		// Returns true if the underlying atomic variables used by
		// the queue are lock-free (they should be on most platforms).
		// Thread-safe.
		static constexpr auto is_lock_free() noexcept{
			return details::static_is_lock_free_v<bool> == 2
				&& details::static_is_lock_free_v<typename Base::size_t> == 2
				&& details::static_is_lock_free_v<std::uint32_t> == 2
				&& details::static_is_lock_free_v<typename Base::index_t> == 2
				&& details::static_is_lock_free_v<void*> == 2
				&& details::static_is_lock_free_v<details::thread_id_converter<details::thread_id_t>::thread_id_numeric_size_t> == 2;
		}

#ifdef MCDBGQ_TRACKMEM

		struct MemStats {
			size_t allocatedBlocks{}
			, usedBlocks{}
			, freeBlocks{}
			, ownedBlocksExplicit{}
			, ownedBlocksImplicit{}
			, implicitProducers{}
			, explicitProducers{}
			, elementsEnqueued{}
			, blockClassBytes{}
			, queueClassBytes{}
			, implicitBlockIndexBytes{}
			, explicitBlockIndexBytes{};

			friend class XConcurrentQueue;

		private:
			static constexpr MemStats getFor(XConcurrentQueue * const q) {
				MemStats stats { };

				stats.elementsEnqueued = q->size_approx();

				for (auto block { q->freeList.head_unsafe() };block ;block = block->freeListNext.loadRelaxed() ) {
					++stats.allocatedBlocks;
					++stats.freeBlocks;
				}

				for (auto ptr{ q->producerListTail.loadAcquire() };
					ptr ; ptr = ptr->next_prod())
				{
					auto const implicit { dynamic_cast<Base::ImplicitProducer*>(ptr) };
					stats.implicitProducers += implicit ? 1 : 0;
					stats.explicitProducers += implicit ? 0 : 1;

					if (implicit) {
						auto const prod { static_cast<Base::ImplicitProducer*>(ptr) };
						stats.queueClassBytes += sizeof(Base::ImplicitProducer);
						auto head { prod->headIndex.loadRelaxed() };
						auto const tail { prod->tailIndex.loadRelaxed() };
						auto hash{ prod->blockIndex.loadRelaxed() };
						if (hash) {
							for (size_t i {}; i != hash->capacity; ++i) {
								if (hash->index[i]->key.loadRelaxed() != Base::ImplicitProducer::INVALID_BLOCK_BASE
									&& hash->index[i]->value.loadRelaxed())
								{
									++stats.allocatedBlocks;
									++stats.ownedBlocksImplicit;
								}
							}
							stats.implicitBlockIndexBytes += hash->capacity * sizeof(Base::ImplicitProducer::BlockIndexEntry);
							for (; hash ; hash = hash->prev)
							{ stats.implicitBlockIndexBytes += sizeof(Base::ImplicitProducer::BlockIndexHeader) + hash->capacity * sizeof(typename Base::ImplicitProducer::BlockIndexEntry *); }
						}
						for (; details::circular_less_than<typename Base::index_t>(head, tail); head += Base::BBLOCK_SIZE) {
							//auto block = prod->get_block_index_entry_for_index(head);
							++stats.usedBlocks;
						}
					} else {
						auto const prod { static_cast<Base::ExplicitProducer*>(ptr) };
						stats.queueClassBytes += sizeof(Base::ExplicitProducer);
						auto const tailBlock{ prod->tailBlock };
						bool wasNonEmpty {};
						if (tailBlock) {
							auto block{ tailBlock };
							do {
								++stats.allocatedBlocks;
								if (!block->Base::Block::template is_empty<Base::explicit_context>() || wasNonEmpty) {
									++stats.usedBlocks;
									wasNonEmpty = wasNonEmpty || block != tailBlock;
								}
								++stats.ownedBlocksExplicit;
								block = block->next;
							} while (block != tailBlock);
						}
						auto index { prod->blockIndex.loadRelaxed() };
						while (index) {
							stats.explicitBlockIndexBytes += sizeof(Base::ExplicitProducer::BlockIndexHeader) + index->size * sizeof(Base::ExplicitProducer::BlockIndexEntry);
							index = static_cast<Base::ExplicitProducer::BlockIndexHeader*>(index->prev);
						}
					}
				}

				auto const freeOnInitialPool {
					q->initialBlockPoolIndex.loadRelaxed() >= q->initialBlockPoolSize
									? 0
									: q->initialBlockPoolSize - q->initialBlockPoolIndex.loadRelaxed()
				};
				stats.allocatedBlocks += freeOnInitialPool;
				stats.freeBlocks += freeOnInitialPool;

				stats.blockClassBytes = sizeof(Base::Block) * stats.allocatedBlocks;
				stats.queueClassBytes += sizeof(XConcurrentQueue);

				return stats;
			}
		};

		// For debugging only. Not thread-safe.
		constexpr MemStats getMemStats() { return MemStats::getFor(this); }

		friend struct MemStats;
#endif

		friend struct ProducerToken;
		friend struct ConsumerToken;
		friend struct ExplicitProducer;
		friend struct ImplicitProducer;
		friend class ConcurrentQueueTests;
		template<typename ,typename > friend class XConcurrentQueueAbstract;
	};
}

#if defined(_MSC_VER) && (!defined(_HAS_CXX17) || !_HAS_CXX17)
#pragma warning(pop)
#endif

#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#pragma GCC diagnostic pop
#endif

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
