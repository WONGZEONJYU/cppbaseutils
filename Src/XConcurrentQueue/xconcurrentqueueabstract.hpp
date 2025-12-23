#ifndef XUTILS2_X_CONCURRENT_QUEUE_ABSTRACT_HPP
#define XUTILS2_X_CONCURRENT_QUEUE_ABSTRACT_HPP 1

#ifndef XUTILS2_X_CONCURRENT_QUEUE_ABSTRACT_HPP_
	#error "xconcurrentqueueabstract is internal header file!"
#endif

#include <cassert>
#include <cstddef>              // for max_align_t
#include <cstdlib>
#include <type_traits>
#include <algorithm>
#include <utility>
#include <limits>
#include <climits>		// for CHAR_BIT
#include <array>
#include <mutex>        // used for thread exit synchronization
#include <XGlobal/xclasshelpermacros.hpp>
#include <XAtomic/xatomic.hpp>
#define XUTILS2_MOODYCAMEL_MACROS_HPP_
#include <XConcurrentQueue/moodycamelmacros.hpp>
#undef XUTILS2_MOODYCAMEL_MACROS_HPP_

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace moodycamel {

    // Default traits for the ConcurrentQueue. To change some of the
	// traits without re-implementing all of them, inherit from this
	// struct and shadow the declarations you wish to be different;
	// since the traits are used as a template type parameter, the
	// shadowed declarations will be used where defined, and the defaults
	// otherwise.
	struct XConcurrentQueueDefaultTraits {
		// General-purpose size type. std::size_t is strongly recommended.
		using size_t = std::size_t;

		// The type used for the enqueue and dequeue indices. Must be at least as
		// large as size_t. Should be significantly larger than the number of elements
		// you expect to hold at once, especially if you have a high turnover rate;
		// for example, on 32-bit x86, if you expect to have over a hundred million
		// elements or pump several million elements through your queue in a very
		// short space of time, using a 32-bit type *may* trigger a race condition.
		// A 64-bit int type is recommended in that case, and in practice will
		// prevent a race condition no matter the usage of the queue. Note that
		// whether the queue is lock-free with a 64-int type depends on the whether
		// std::atomic<std::uint64_t> is lock-free, which is platform-specific.
		using index_t = std::size_t ;

		// Internally, all elements are enqueued and dequeued from multi-element
		// blocks; this is the smallest controllable unit. If you expect few elements
		// but many producers, a smaller block size should be favoured. For few producers
		// and/or many elements, a larger block size is preferred. A sane default
		// is provided. Must be a power of 2.
		static constexpr size_t BLOCK_SIZE {32}

		// For explicit producers (i.e. when using a producer token), the block is
		// checked for being empty by iterating through a list of flags, one per element.
		// For large block sizes, this is too inefficient, and switching to an atomic
		// counter-based approach is faster. The switch is made for block sizes strictly
		// larger than this threshold.
		, EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD { 32 }

		// How many full blocks can be expected for a single explicit producer? This should
		// reflect that number's maximum for optimal performance. Must be a power of 2.
		, EXPLICIT_INITIAL_INDEX_SIZE {32}

		// How many full blocks can be expected for a single implicit producer? This should
		// reflect that number's maximum for optimal performance. Must be a power of 2.
		, IMPLICIT_INITIAL_INDEX_SIZE {32}

		// The initial size of the hash table mapping thread IDs to implicit producers.
		// Note that the hash is resized every time it becomes half full.
		// Must be a power of two, and either 0 or at least 1. If 0, implicit production
		// (using the enqueue methods without an explicit producer token) is disabled.
		, INITIAL_IMPLICIT_PRODUCER_HASH_SIZE {32};

		// Controls the number of items that an explicit consumer (i.e. one with a token)
		// must consume before it causes all consumers to rotate and move on to the next
		// internal queue.
		static constexpr std::uint32_t EXPLICIT_CONSUMER_CONSUMPTION_QUOTA_BEFORE_ROTATE {256};

		// The maximum number of elements (inclusive) that can be enqueued to a sub-queue.
		// Enqueue operations that would cause this limit to be surpassed will fail. Note
		// that this limit is enforced at the block level (for performance reasons), i.e.
		// it's rounded up to the nearest block size.
		static constexpr size_t MAX_SUBQUEUE_SIZE { moodycamel::details::const_numeric_max_v<size_t> };

		// The number of times to spin before sleeping when waiting on a semaphore.
		// Recommended values are on the order of 1000-10000 unless the number of
		// consumer threads exceeds the number of idle cores (in which case try 0-100).
		// Only affects instances of the BlockingConcurrentQueue.
		static constexpr auto MAX_SEMA_SPINS {10000};

		// Whether to recycle dynamically-allocated blocks into an internal free list or
		// not. If false, only pre-allocated blocks (controlled by the constructor
		// arguments) will be recycled, and all others will be `free`d back to the heap.
		// Note that blocks consumed by explicit producers are only freed on destruction
		// of the queue (not following destruction of the token) regardless of this trait.
		static constexpr auto RECYCLE_ALLOCATED_BLOCKS { false };

#ifndef MCDBGQ_USE_RELACY
		// Memory allocation can be customized if needed.
		// malloc should return nullptr on failure, and handle alignment like std::malloc.
#if defined(malloc) || defined(free)
		// Gah, this is 2015, stop defining macros that break standard code already!
		// Work around malloc/free being special macros:
		static void * WORKAROUND_malloc(size_t const size) { return malloc(size); }
		static void WORKAROUND_free(void * const ptr) { return free(ptr); }
		static void * (malloc)(size_t const size) { return WORKAROUND_malloc(size); }
		static void (free)(void * const ptr) { return WORKAROUND_free(ptr); }
#else
		static void * malloc(size_t const size) { return std::malloc(size); }
		static void free(void * const ptr) { return std::free(ptr); }
#endif
#else
		// Debug versions when running under the Relacy race detector (ignore
		// these in user code)
		static void * malloc(size_t const size) { return rl::rl_malloc(size, $); }
		static void free(void * const ptr) { return rl::rl_free(ptr, $); }
#endif
	};

	struct ProducerToken;
	struct ConsumerToken;
	class XConcurrentQueueTests;

	template<typename T, typename Traits> class XConcurrentQueueAbstract;
	template<typename T, typename Traits> class XConcurrentQueue;
	template<typename T, typename Traits> class XBlockingConcurrentQueueAbstract;
	template<typename T, typename Traits> class XBlockingConcurrentQueue;

	namespace details {
		// When producing or consuming many elements, the most efficient way is to:
		//    1) Use one of the bulk-operation methods of the queue with a token
		//    2) Failing that, use the bulk-operation methods without a token
		//    3) Failing that, create a token and use that with the single-item methods
		//    4) Failing that, use the single-parameter methods of the queue
		// Having said that, don't create tokens willy-nilly -- ideally there should be
		// a maximum of one token per thread (of each kind).

		struct ConcurrentQueueProducerTypelessBase {
			ConcurrentQueueProducerTypelessBase * next{};
			XAtomicBool inactive{};
			ProducerToken * token {};
			constexpr ConcurrentQueueProducerTypelessBase() = default;
		};

		template<bool> struct _hash_32_or_64 {
			static constexpr std::uint32_t hash(std::uint32_t h) noexcept {
				// MurmurHash3 finalizer -- see https://code.google.com/p/smhasher/source/browse/trunk/MurmurHash3.cpp
				// Since the thread ID is already unique, all we really want to do is propagate that
				// uniqueness evenly across all the bits, so that we can use a subset of the bits while
				// reducing collisions significantly
				h ^= h >> 16;h *= 0x85ebca6b;
				h ^= h >> 13;h *= 0xc2b2ae35;
				return h ^ h >> 16;
			}
		};

		template<> struct _hash_32_or_64<true> {
			static constexpr std::uint64_t hash(std::uint64_t h) noexcept {
				h ^= h >> 33;h *= 0xff51afd7ed558ccd;
				h ^= h >> 33;h *= 0xc4ceb9fe1a85ec53;
				return h ^ h >> 33;
			}
		};

		template<std::size_t size> struct hash_32_or_64 : _hash_32_or_64<(size > 4)> {  };

		[[maybe_unused]] static constexpr std::size_t hash_thread_id(thread_id_t const & id) {
			static_assert(sizeof(thread_id_t) <= 8, "Expected a platform where thread IDs are at most 64-bit values");
			return hash_32_or_64<sizeof(thread_id_converter<thread_id_t>::thread_id_hash_t)>::hash(
				thread_id_converter<thread_id_t>::preHash(id));
		}

		template<typename T>
		static constexpr bool circular_less_than(T a, T b) noexcept {
			static_assert(std::is_integral_v<T> && !std::numeric_limits<T>::is_signed, "circular_less_than is intended to be used only with unsigned integer types");
			auto constexpr bits { static_cast<T>(sizeof(T) * CHAR_BIT - 1) };
			auto const left { static_cast<T>(a - b) };
			auto const right { static_cast<T>( static_cast<T>(1) << bits ) };
			return left > right;
			// Note: extra parens around rhs of operator<< is MSVC bug: https://developercommunity2.visualstudio.com/t/C4554-triggers-when-both-lhs-and-rhs-is/10034931
			//       silencing the bug requires #pragma warning(disable: 4554) around the calling code and has no effect when done here.
		}

		template<typename U>
		static constexpr char * align_for(char * const ptr) noexcept {
			auto constexpr alignment{std::alignment_of_v<U>};
			return ptr + (alignment - reinterpret_cast<std::uintptr_t>(ptr) % alignment) % alignment;
		}

		template<typename T>
		static constexpr T ceil_to_pow_2(T x) noexcept {
			static_assert(std::is_integral_v<T> && !std::numeric_limits<T>::is_signed, "ceil_to_pow_2 is intended to be used only with unsigned integer types");
			// Adapted from http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
			--x;
			x |= x >> 1;
			x |= x >> 2;
			x |= x >> 4;
			for (std::size_t i {1}; i < sizeof(T); i <<= 1)
			{ x |= x >> (i << 3); }
			++x;
			return x;
		}

		template<typename T>
		static constexpr void swap_relaxed(std::atomic<T> & left, std::atomic<T> & right) noexcept {
			auto const temp{ left.load(std::memory_order_relaxed) };
			left.store(right.load(std::memory_order_relaxed), std::memory_order_relaxed);
			right.store(temp, std::memory_order_relaxed);
		}

		template<typename T>
		static constexpr T const & nomove(T const & x) { return x; }

		template<bool>
		struct nomove_if {
			template<typename T>
			static constexpr T const & eval(T const & x) { return x; }
		};

		template<>
		struct nomove_if<false> {
			template<typename U>
			static constexpr auto eval(U && x) -> decltype(std::forward<U>(x))
			{ return std::forward<U>(x); }
		};

		template<typename It>
		static constexpr auto deref_noexcept(It && it) noexcept -> decltype(*it)
		{ return *it; }

#if defined(__clang__) || !defined(__GNUC__) || __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)
		template<typename T> struct is_trivially_destructible : std::is_trivially_destructible<T> { };
#else
		template<typename T> struct is_trivially_destructible : std::has_trivial_destructor<T> { };
#endif
		template<typename T> using is_trivially_destructible_t = is_trivially_destructible<T>::value_type;
		template<typename T> inline constexpr auto is_trivially_destructible_v { is_trivially_destructible<T>::value };

#ifdef MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED
#ifdef MCDBGQ_USE_RELACY
		using ThreadExitListener = RelacyThreadExitListener;
		using ThreadExitNotifier = RelacyThreadExitNotifier;
#else

		class ThreadExitNotifier;

		struct ThreadExitListener {
			using callback_t = void (*)(void*);
			callback_t callback {};
			void * userData {};

			ThreadExitListener * next{};		// reserved for use by the ThreadExitNotifier
			ThreadExitNotifier * chain{};		// reserved for use by the ThreadExitNotifier
		};

		class ThreadExitNotifier {
		public:
			static void subscribe(ThreadExitListener * const listener) {
				auto && tlsInst{ instance() };
				std::unique_lock lk { mutex() };
				listener->next = tlsInst.tail;
				listener->chain = std::addressof(tlsInst);
				tlsInst.tail = listener;
			}

			static void unsubscribe(ThreadExitListener * const listener) {
				std::unique_lock lk { mutex() };
				if (!listener->chain) { return; } // race with ~ThreadExitNotifier
				auto && tlsInst{ *listener->chain };
				listener->chain = {};
				auto prev{ std::addressof(tlsInst.tail) };
				for (auto ptr{ tlsInst.tail }; ptr; ptr = ptr->next) {
					if (listener == ptr) { *prev = ptr->next; break; }
					prev = std::addressof(ptr->next);
				}
			}

		private:
			constexpr ThreadExitNotifier() = default;

			X_DISABLE_COPY(ThreadExitNotifier)

			~ThreadExitNotifier() {
				// This thread is about to exit, let everyone know!
				assert(this == std::addressof(instance())
					&& "If this assert fails, you likely have a buggy compiler! Change the preprocessor conditions such that MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED is no longer defined.");
				std::unique_lock lk {mutex()};
				for (auto ptr{tail}; ptr; ptr = ptr->next) {
					ptr->chain = {};
					ptr->callback(ptr->userData);
				}
			}

			// Thread-local
			static ThreadExitNotifier & instance() noexcept {
				thread_local ThreadExitNotifier notifier;
				return notifier;
			}

			static std::mutex & mutex() noexcept {
				// Must be static because the ThreadExitNotifier could be destroyed while unsubscribe is called
				static std::mutex mutex{};
				return mutex;
			}

			ThreadExitListener * tail{};
		};
#endif
#endif

		template<typename> struct static_is_lock_free_num { enum { value }; };
		template<> struct static_is_lock_free_num<signed char> { enum { value = ATOMIC_CHAR_LOCK_FREE }; };
		template<> struct static_is_lock_free_num<short> { enum { value = ATOMIC_SHORT_LOCK_FREE }; };
		template<> struct static_is_lock_free_num<int> { enum { value = ATOMIC_INT_LOCK_FREE }; };
		template<> struct static_is_lock_free_num<long> { enum { value = ATOMIC_LONG_LOCK_FREE }; };
		template<> struct static_is_lock_free_num<long long> { enum { value = ATOMIC_LLONG_LOCK_FREE }; };
		template<typename T> struct static_is_lock_free : static_is_lock_free_num<std::make_signed_t<T>> { };
		template<> struct static_is_lock_free<bool> { enum { value = ATOMIC_BOOL_LOCK_FREE }; };
		template<typename U> struct static_is_lock_free<U*> { enum { value = ATOMIC_POINTER_LOCK_FREE }; };
		template<typename T> inline constexpr auto static_is_lock_free_v { static_is_lock_free<T>::value };

	}

	struct ProducerToken {
		template<typename T,typename Traits>
		explicit ProducerToken(XConcurrentQueueAbstract<T, Traits> & queue)
			: producer(queue.recycle_or_create_producer(true))
		{ if (producer) { producer->token = this; } }

		template<typename T,typename Traits>
		explicit ProducerToken(XBlockingConcurrentQueue<T, Traits> & queue)
			: producer(reinterpret_cast<XConcurrentQueueAbstract<T, Traits>*>(std::addressof(queue))->recycle_or_create_producer(true))
		{ if (producer) { producer->token = this; } }

		ProducerToken(ProducerToken && other) noexcept : producer {other.producer}
		{ other.producer = {}; if (producer) { producer->token = this; } }

		ProducerToken & operator=(ProducerToken && other) noexcept
		{ swap(other); return *this; }

		constexpr void swap(ProducerToken & other) noexcept {
			std::swap(producer, other.producer);
			if (producer) { producer->token = this; }
			if (other.producer) { other.producer->token = std::addressof(other); }
		}

		// A token is always valid unless:
		//     1) Memory allocation failed during construction
		//     2) It was moved via the move constructor
		//        (Note: assignment does a swap, leaving both potentially valid)
		//     3) The associated queue was destroyed
		// Note that if valid() returns true, that only indicates
		// that the token is valid for use with a specific queue,
		// but not which one; that's up to the user to track.
		[[nodiscard]] constexpr bool valid() const noexcept { return producer; }

		~ProducerToken() {
			if (producer) {
				producer->token = {};
				producer->inactive.storeRelease(true);
			}
		}

		// Disable copying and assignment
		X_DISABLE_COPY(ProducerToken)

	private:
		friend class XConcurrentQueueTests;
		template<typename, typename > friend class XConcurrentQueue;
		template<typename, typename > friend class XConcurrentQueueAbstract;

	protected:
		using ConcurrentQueueProducerTypelessBase = details::ConcurrentQueueProducerTypelessBase;
		ConcurrentQueueProducerTypelessBase * producer{};
	};

	struct ConsumerToken {

		template<typename T, typename Traits>
		explicit ConsumerToken(XConcurrentQueue<T, Traits> & q) {
			initialOffset = q.nextExplicitConsumerId.fetchAndAddRelease(1);
			lastKnownGlobalOffset = static_cast<std::uint32_t>(-1);
		}

		template<typename T, typename Traits>
		explicit ConsumerToken(XBlockingConcurrentQueue<T, Traits> & q) {
			initialOffset = reinterpret_cast<XConcurrentQueue<T, Traits>*>(std::addressof(q))->nextExplicitConsumerId.fetchAndAddRelease(1);
			lastKnownGlobalOffset = static_cast<std::uint32_t>(-1);
		}

		ConsumerToken(ConsumerToken && other) MOODYCAMEL_NOEXCEPT
			: initialOffset(other.initialOffset), lastKnownGlobalOffset(other.lastKnownGlobalOffset)
			, itemsConsumedFromCurrent(other.itemsConsumedFromCurrent)
			, currentProducer(other.currentProducer), desiredProducer(other.desiredProducer)
		{}

		ConsumerToken& operator=(ConsumerToken&& other) MOODYCAMEL_NOEXCEPT
		{ swap(other); return *this; }

		constexpr void swap(ConsumerToken& other) MOODYCAMEL_NOEXCEPT {
			std::swap(initialOffset, other.initialOffset);
			std::swap(lastKnownGlobalOffset, other.lastKnownGlobalOffset);
			std::swap(itemsConsumedFromCurrent, other.itemsConsumedFromCurrent);
			std::swap(currentProducer, other.currentProducer);
			std::swap(desiredProducer, other.desiredProducer);
		}

		// Disable copying and assignment
		X_DISABLE_COPY(ConsumerToken)

	private:
		template<typename T, typename Traits> friend class XConcurrentQueue;
		template<typename T, typename Traits> friend class XConcurrentQueueAbstract;
		friend class XConcurrentQueueTests;

		// but shared with ConcurrentQueue
		std::uint32_t initialOffset {},lastKnownGlobalOffset {},itemsConsumedFromCurrent {};
		using ConcurrentQueueProducerTypelessBase = details::ConcurrentQueueProducerTypelessBase;
		ConcurrentQueueProducerTypelessBase * currentProducer{},* desiredProducer{};
	};

	template<typename T, typename Traits>
	constexpr void swap(XConcurrentQueue<T, Traits>& a, XConcurrentQueue<T, Traits>& b) MOODYCAMEL_NOEXCEPT
	{ a.swap(b); }

	template<typename T, typename Traits>
	constexpr void swap(XConcurrentQueueAbstract<T, Traits> & a, XConcurrentQueueAbstract<T, Traits> & b) MOODYCAMEL_NOEXCEPT
	{ a.swap(b); }

	[[maybe_unused]] static constexpr void swap(ProducerToken & a, ProducerToken & b) MOODYCAMEL_NOEXCEPT
	{ a.swap(b); }

	[[maybe_unused]] static constexpr void swap(ConsumerToken & a, ConsumerToken & b) MOODYCAMEL_NOEXCEPT
	{ a.swap(b); }

	// Need to forward-declare this swap because it's in a namespace.
	// See http://stackoverflow.com/questions/4492062/why-does-a-c-friend-class-need-a-forward-declaration-only-in-other-namespaces
	template<typename T, typename Traits>
	constexpr void swap(typename XConcurrentQueue<T, Traits>::ImplicitProducerKVP & a
		, typename XConcurrentQueue<T, Traits>::ImplicitProducerKVP & b) MOODYCAMEL_NOEXCEPT
	{ a.swap(b); }

	template<typename T, typename Traits>
	constexpr void swap(typename XConcurrentQueueAbstract<T, Traits>::ImplicitProducerKVP & a
		, typename XConcurrentQueueAbstract<T, Traits>::ImplicitProducerKVP & b) MOODYCAMEL_NOEXCEPT
	{ a.swap(b); }

	template<typename T, typename Traits>
	class XConcurrentQueueAbstract {
		template<typename ,typename > friend class XConcurrentQueue;

	public:
		using value_type = T;
		using producer_token_t = ProducerToken ;
		using consumer_token_t = ConsumerToken ;

		using index_t = Traits::index_t ;
		using size_t = Traits::size_t ;

		static constexpr auto BLOCK_SIZE { static_cast<size_t>(Traits::BLOCK_SIZE) }
			,EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD { static_cast<size_t>(Traits::EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD) }
			,EXPLICIT_INITIAL_INDEX_SIZE { static_cast<size_t>(Traits::EXPLICIT_INITIAL_INDEX_SIZE) }
			,IMPLICIT_INITIAL_INDEX_SIZE { static_cast<size_t>(Traits::IMPLICIT_INITIAL_INDEX_SIZE) }
			,INITIAL_IMPLICIT_PRODUCER_HASH_SIZE { static_cast<size_t>(Traits::INITIAL_IMPLICIT_PRODUCER_HASH_SIZE) }
		;

		static constexpr auto EXPLICIT_CONSUMER_CONSUMPTION_QUOTA_BEFORE_ROTATE
		{ static_cast<std::uint32_t>(Traits::EXPLICIT_CONSUMER_CONSUMPTION_QUOTA_BEFORE_ROTATE) };

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4307)		// + integral constant overflow (that's what the ternary expression is for!)
#pragma warning(disable: 4309)		// static_cast: Truncation of constant value
#endif
		static constexpr auto MAX_SUBQUEUE_SIZE {
			details::const_numeric_max<size_t>::value - static_cast<size_t>(Traits::MAX_SUBQUEUE_SIZE) < BLOCK_SIZE
			   ? details::const_numeric_max<size_t>::value
			   : (static_cast<size_t>(Traits::MAX_SUBQUEUE_SIZE) + (BLOCK_SIZE - 1)) / BLOCK_SIZE * BLOCK_SIZE
	   };
#ifdef _MSC_VER
#pragma warning(pop)
#endif

		static_assert(!std::numeric_limits<size_t>::is_signed && std::is_integral_v<size_t>, "Traits::size_t must be an unsigned integral type");
		static_assert(!std::numeric_limits<index_t>::is_signed && std::is_integral_v<index_t>, "Traits::index_t must be an unsigned integral type");
		static_assert(sizeof(index_t) >= sizeof(size_t), "Traits::index_t must be at least as wide as Traits::size_t");
		static_assert(BLOCK_SIZE > 1 && !(BLOCK_SIZE & BLOCK_SIZE - 1), "Traits::BLOCK_SIZE must be a power of 2 (and at least 2)");
		static_assert(EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD > 1 && !(EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD & EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD - 1), "Traits::EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD must be a power of 2 (and greater than 1)");
		static_assert(EXPLICIT_INITIAL_INDEX_SIZE > 1 && !(EXPLICIT_INITIAL_INDEX_SIZE & EXPLICIT_INITIAL_INDEX_SIZE - 1), "Traits::EXPLICIT_INITIAL_INDEX_SIZE must be a power of 2 (and greater than 1)");
		static_assert(IMPLICIT_INITIAL_INDEX_SIZE > 1 && !(IMPLICIT_INITIAL_INDEX_SIZE & IMPLICIT_INITIAL_INDEX_SIZE - 1), "Traits::IMPLICIT_INITIAL_INDEX_SIZE must be a power of 2 (and greater than 1)");
		static_assert(INITIAL_IMPLICIT_PRODUCER_HASH_SIZE == 0 || !(INITIAL_IMPLICIT_PRODUCER_HASH_SIZE & INITIAL_IMPLICIT_PRODUCER_HASH_SIZE - 1), "Traits::INITIAL_IMPLICIT_PRODUCER_HASH_SIZE must be a power of 2");
		static_assert(INITIAL_IMPLICIT_PRODUCER_HASH_SIZE == 0 || INITIAL_IMPLICIT_PRODUCER_HASH_SIZE >= 1, "Traits::INITIAL_IMPLICIT_PRODUCER_HASH_SIZE must be at least 1 (or 0 to disable implicit enqueueing)");

	private:
		enum AllocationMode { CanAlloc, CannotAlloc };
		enum InnerQueueContext { implicit_context , explicit_context };

		///////////////////////////
		// Free list
		///////////////////////////
		template <typename N>
		struct FreeListNode {
			constexpr FreeListNode() = default;
			XAtomicInteger<std::uint32_t> freeListRefs {};
			XAtomicPointer<N> freeListNext {};
		};

		// A simple CAS-based lock-free free list. Not the fastest thing in the world under heavy contention, but
		// simple and correct (assuming nodes are never freed until after the free list is destroyed), and fairly
		// speedy under low contention.
		template<typename N>		// N must inherit FreeListNode or have the same fields (and initialization of them)
		struct FreeList {
			constexpr FreeList() = default;

			FreeList(FreeList && other) noexcept
			{ swap(other); }

			constexpr void swap(FreeList & other) noexcept
			{ details::swap_relaxed(freeListHead.m_x_value, other.freeListHead.m_x_value); }

			X_DISABLE_COPY(FreeList)

			constexpr void add(N * node) {
#ifdef MCDBGQ_NOLOCKFREE_FREELIST
				debug::DebugLock lock { mutex };
#endif
				// We know that the should-be-on-freelist bit is 0 at this point, so it's safe to
				// set it using a fetch_add
				if (!node->freeListRefs.fetchAndAddOrdered(SHOULD_BE_ON_FREELIST)) {
					// Oh look! We were the last ones referencing this node, and we know
					// we want to add it to the free list, so let's do it!
					add_knowing_refcount_is_zero(node);
				}
			}

			constexpr N * try_get() {
#ifdef MCDBGQ_NOLOCKFREE_FREELIST
				debug::DebugLock lock { mutex };
#endif
				auto head { freeListHead.loadAcquire() };
				while (head) {
					auto const prevHead { head };
					auto refs { head->freeListRefs.loadRelaxed() };
					if (!(refs & REFS_MASK) || !head->freeListRefs.m_x_value.compare_exchange_strong(refs, refs + 1, std::memory_order_acquire)) {
						head = freeListHead.loadAcquire();
						continue;
					}

					// Good, reference count has been incremented (it wasn't at zero), which means we can read the
					// next and not worry about it changing between now and the time we do the CAS

					if (auto const next { head->freeListNext.loadRelaxed() };
						 freeListHead.m_x_value.compare_exchange_strong(head, next, std::memory_order_acquire, std::memory_order_relaxed) )
					{
						// Yay, got the node. This means it was on the list, which means shouldBeOnFreeList must be false no
						// matter the refcount (because nobody else knows it's been taken off yet, it can't have been put back on).
						assert(!(head->freeListRefs.loadRelaxed() & SHOULD_BE_ON_FREELIST));

						// Decrease refcount twice, once for our ref, and once for the list's ref
						head->freeListRefs.fetchAndSubRelease(2);
						return head;
					}

					// OK, the head must have changed on us, but we still need to decrease the refcount we increased.
					// Note that we don't need to release any memory effects, but we do need to ensure that the reference
					// count decrement happens-after the CAS on the head.
					refs = prevHead->freeListRefs.fetchAndSubOrdered(1);
					if (refs == SHOULD_BE_ON_FREELIST + 1) { add_knowing_refcount_is_zero(prevHead); }
				}

				return {};
			}

			// Useful for traversing the list when there's no contention (e.g. to destroy remaining nodes)
			constexpr N * head_unsafe() const noexcept
			{ return freeListHead.loadRelaxed(); }

		private:
			constexpr void add_knowing_refcount_is_zero(N * const node) noexcept {
				// Since the refcount is zero, and nobody can increase it once it's zero (except us, and we run
				// only one copy of this method per node at a time, i.e. the single thread case), then we know
				// we can safely change the next pointer of the node; however, once the refcount is back above
				// zero, then other threads could increase it (happens under heavy contention, when the refcount
				// goes to zero in between a load and a refcount increment of a node in try_get, then back up to
				// something non-zero, then the refcount increment is done by the other thread) -- so, if the CAS
				// to add the node to the actual list fails, decrease the refcount and leave the add operation to
				// the next thread who puts the refcount back at zero (which could be us, hence the loop).
				auto head { freeListHead.loadRelaxed() };
				while (true) {
					node->freeListNext.storeRelaxed(head);
					node->freeListRefs.storeRelease(1);
					if (!freeListHead.m_x_value.compare_exchange_strong(head, node, std::memory_order_release, std::memory_order_relaxed)) {
						// Hmm, the add failed, but we can only try again when the refcount goes back to zero
						if (node->freeListRefs.fetchAndAddOrdered(SHOULD_BE_ON_FREELIST - 1) == 1)
						{ continue; }
					}
					return;
				}
			}

			// Implemented like a stack, but where node order doesn't matter (nodes are inserted out of order under contention)
			XAtomicPointer<N> freeListHead {};
			static constexpr std::uint32_t REFS_MASK { 0x7FFFFFFF},SHOULD_BE_ON_FREELIST { 0x80000000 };

#ifdef MCDBGQ_NOLOCKFREE_FREELIST
			debug::DebugMutex mutex{};
#endif
		};

		///////////////////////////
		// Block
		///////////////////////////
		struct Block {

			constexpr Block() : dynamicallyAllocated { true } {}

			template<InnerQueueContext context>
			constexpr bool is_empty() const noexcept {
				MOODYCAMEL_CONSTEXPR_IF (context == explicit_context && BLOCK_SIZE <= EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD) {
					// Check flags
					for (size_t i {}; i < BLOCK_SIZE; ++i)
					{ if (!emptyFlags[i].loadRelaxed()) { return {}; } }

					// Aha, empty; make sure we have all other memory effects that happened before the empty flags were set
					std::atomic_thread_fence(std::memory_order_acquire);
					return true;
				}else {
					// Check counter
					if (elementsCompletelyDequeued.loadRelaxed() == BLOCK_SIZE) {
						std::atomic_thread_fence(std::memory_order_acquire);
						return true;
					}
					assert(elementsCompletelyDequeued.loadRelaxed() <= BLOCK_SIZE);
					return {};
				}
			}

			// Returns true if the block is now empty (does not apply in explicit context)
			template<InnerQueueContext context>
			constexpr bool set_empty(MOODYCAMEL_MAYBE_UNUSED index_t const i) noexcept {
				MOODYCAMEL_CONSTEXPR_IF (context == explicit_context && BLOCK_SIZE <= EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD) {
					// Set flag
					assert(!emptyFlags[BLOCK_SIZE - 1 - static_cast<size_t>(i & static_cast<index_t>(BLOCK_SIZE - 1))].loadRelaxed());
					emptyFlags[BLOCK_SIZE - 1 - static_cast<size_t>(i & static_cast<index_t>(BLOCK_SIZE - 1))].storeRelease(true);
					return {};
				}else {
					// Increment counter
					auto const prevVal{ elementsCompletelyDequeued.fetchAndAddOrdered(1) };
					assert(prevVal < BLOCK_SIZE);
					return prevVal == BLOCK_SIZE - 1;
				}
			}

			// Sets multiple contiguous item statuses to 'empty' (assumes no wrapping and count > 0).
			// Returns true if the block is now empty (does not apply in explicit context).
			template<InnerQueueContext context>
			constexpr bool set_many_empty(MOODYCAMEL_MAYBE_UNUSED index_t i, size_t const count) noexcept {
				MOODYCAMEL_CONSTEXPR_IF (context == explicit_context && BLOCK_SIZE <= EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD) {
					// Set flags
					std::atomic_thread_fence(std::memory_order_release);
					i = BLOCK_SIZE - 1 - static_cast<size_t>(i & static_cast<index_t>(BLOCK_SIZE - 1)) - count + 1;
					for (size_t j {}; j != count; ++j) {
						assert(!emptyFlags[i + j].loadRelaxed());
						emptyFlags[i + j].storeRelaxed(true);
					}
					return {};
				} else {
					// Increment counter
					auto const prevVal{ elementsCompletelyDequeued.fetchAndAddOrdered(count)};
					assert(prevVal + count <= BLOCK_SIZE);
					return prevVal + count == BLOCK_SIZE;
				}
			}

			template<InnerQueueContext context>
			constexpr void set_all_empty() noexcept {
				MOODYCAMEL_CONSTEXPR_IF (context == explicit_context && BLOCK_SIZE <= EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD) {
					// Set all flags
					for (size_t i {}; i != BLOCK_SIZE; ++i)
					{ emptyFlags[i].storeRelaxed(true); }
				}
				else {
					// Reset counter
					elementsCompletelyDequeued.storeRelaxed(BLOCK_SIZE);
				}
			}

			template<InnerQueueContext context>
			constexpr void reset_empty() noexcept {
				MOODYCAMEL_CONSTEXPR_IF (context == explicit_context && BLOCK_SIZE <= EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD) {
					// Reset flags
					for (size_t i {}; i != BLOCK_SIZE; ++i)
					{ emptyFlags[i].storeRelaxed({}); }
				}
				else {
					// Reset counter
					elementsCompletelyDequeued.storeRelaxed({});
				}
			}

			constexpr T * operator[](index_t const idx) MOODYCAMEL_NOEXCEPT
			{ return static_cast<T*>(static_cast<void*>(elements)) + static_cast<size_t>(idx & static_cast<index_t>(BLOCK_SIZE - 1)); }

			constexpr T const * operator[](index_t const idx) const MOODYCAMEL_NOEXCEPT
			{ return static_cast<T const*>(static_cast<void const*>(elements)) + static_cast<size_t>(idx & static_cast<index_t>(BLOCK_SIZE - 1)); }

		private:
			static_assert(std::alignment_of_v<T> <= sizeof(T), "The queue does not support types with an alignment greater than their size at this time");
			MOODYCAMEL_ALIGNED_TYPE_LIKE(char[sizeof(T) * BLOCK_SIZE], T) elements {};

		public:
			Block * next { nullptr };
			XAtomicInteger<size_t> elementsCompletelyDequeued {};
			XAtomicBool emptyFlags[BLOCK_SIZE <= EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD ? BLOCK_SIZE : 1] {};

			XAtomicInteger<std::uint32_t> freeListRefs {};
			XAtomicPointer<Block> freeListNext {};
			bool dynamicallyAllocated {};		// Perhaps a better name for this would be 'isNotPartOfInitialBlockPool'

#ifdef MCDBGQ_TRACKMEM
			void * owner{};
#endif
		};
		static_assert(std::alignment_of_v<Block> >= std::alignment_of_v<T>, "Internal error: Blocks must be at least as aligned as the type they are wrapping");

		///////////////////////////
		// Producer base
		///////////////////////////
		struct ProducerBase : details::ConcurrentQueueProducerTypelessBase {

			ProducerBase(XConcurrentQueueAbstract * const parent_, bool const isExplicit_)
			: isExplicit(isExplicit_),parent(parent_) {}

			virtual ~ProducerBase() = default;

			template<typename U>
			constexpr bool dequeue(U & element) {
				return isExplicit
					? static_cast<ExplicitProducer*>(this)->dequeue(element)
					: static_cast<ImplicitProducer*>(this)->dequeue(element);
			}

			template<typename It>
			constexpr size_t dequeue_bulk(It && itemFirst, size_t const max) {
				return isExplicit
						? static_cast<ExplicitProducer*>(this)->dequeue_bulk(std::forward<decltype(itemFirst)>(itemFirst), max)
						: static_cast<ImplicitProducer*>(this)->dequeue_bulk(std::forward<decltype(itemFirst)>(itemFirst), max);
			}

			constexpr ProducerBase * next_prod() const noexcept { return static_cast<ProducerBase*>(next); }

			constexpr size_t size_approx() const noexcept {
				auto const head{ headIndex.loadRelaxed() }
				,tail{ tailIndex.loadRelaxed() };
				return details::circular_less_than(head, tail) ? static_cast<size_t>(tail - head) : 0;
			}

			constexpr auto getTail() const noexcept
			{ return tailIndex.loadRelaxed(); }

		protected:
			XAtomicInteger<index_t> tailIndex{}		// Where to enqueue to next
									, headIndex{}		// Where to dequeue from next
									, dequeueOptimisticCount{}
									,dequeueOvercommit {};
			Block * tailBlock {};

		public:
			bool isExplicit {};
			XConcurrentQueueAbstract * parent{};
			friend struct MemStats;
		};

		///////////////////////////
		// Explicit queue
		///////////////////////////
		struct ExplicitProducer : ProducerBase {
			explicit ExplicitProducer(XConcurrentQueueAbstract * const parent_)
			: ProducerBase(parent_, true) ,pr_blockIndexSize {EXPLICIT_INITIAL_INDEX_SIZE >> 1  }
			{
				if (auto const poolBasedIndexSize {details::ceil_to_pow_2(parent_->initialBlockPoolSize) >> 1};
					poolBasedIndexSize > pr_blockIndexSize)
				{ pr_blockIndexSize = poolBasedIndexSize; }
				new_block_index({});		// This creates an index with double the number of current entries, i.e. EXPLICIT_INITIAL_INDEX_SIZE
			}

			~ExplicitProducer() override {
				// Destruct any elements not yet dequeued.
				// Since we're in the destructor, we can assume all elements
				// are either completely dequeued or completely not (no halfways).
				if (this->tailBlock) {		// Note this means there must be a block index too
					// First find the block that's partially dequeued, if any
					Block * halfDequeuedBlock {};
					if (this->headIndex.loadRelaxed() & static_cast<index_t>(BLOCK_SIZE - 1)) {
						// The head's not on a block boundary, meaning a block somewhere is partially dequeued
						// (or the head block is the tail block and was fully dequeued, but the head/tail are still not on a boundary)
						auto i{ pr_blockIndexFront - pr_blockIndexSlotsUsed & pr_blockIndexSize - 1 };
						while (details::circular_less_than<index_t>(pr_blockIndexEntries[i].base + BLOCK_SIZE, this->headIndex.loadRelaxed()))
						{ i = i + 1 & pr_blockIndexSize - 1; }
						assert( details::circular_less_than<index_t>(pr_blockIndexEntries[i].base,this->headIndex.loadRelaxed()) );
						halfDequeuedBlock = pr_blockIndexEntries[i].block;
					}

					// Start at the head block (note the first line in the loop gives us the head from the tail on the first iteration)
					auto block {this->tailBlock};
					do {
						block = block->next;
						if (block->XConcurrentQueueAbstract::Block::template is_empty<explicit_context>())  { continue; }

						size_t i {};	// Offset into block
						if (block == halfDequeuedBlock)
						{ i = static_cast<size_t>(this->headIndex.loadRelaxed() & static_cast<index_t>(BLOCK_SIZE - 1)); }

						// Walk through all the items in the block; if this is the tail block, we need to stop when we reach the tail index

						auto const lastValidIndex{ this->tailIndex.loadRelaxed() & static_cast<index_t>(BLOCK_SIZE - 1)
								? static_cast<size_t>(this->tailIndex.loadRelaxed() & static_cast<index_t>(BLOCK_SIZE - 1))
								: BLOCK_SIZE
						};

						while (BLOCK_SIZE != i && (block != this->tailBlock || lastValidIndex != i ))
						{ (*block)[i++]->~T(); }
					} while (block != this->tailBlock);
				}

				// Destroy all blocks that we own
				if (auto block { this->tailBlock }) {
					do {
						auto const nextBlock { block->next };
						this->parent->add_block_to_free_list(block);
						block = nextBlock;
					} while (block != this->tailBlock);
				}

				// Destroy the block indices
				auto header { static_cast<BlockIndexHeader*>(pr_blockIndexRaw) };
				while (header) {
					auto const prev { static_cast<BlockIndexHeader*>(header->prev) };
					header->~BlockIndexHeader();
					(Traits::free)(header);
					header = prev;
				}
			}

			template<AllocationMode allocMode, typename U>
			constexpr bool enqueue(U && element) {
				auto const currentTailIndex{ this->tailIndex.loadRelaxed() };
				auto const newTailIndex{ 1 + currentTailIndex };
				if (!(currentTailIndex & static_cast<index_t>(BLOCK_SIZE - 1))) {
					// We reached the end of a block, start a new one
					auto const startBlock { this->tailBlock };
					auto const originalBlockIndexSlotsUsed{pr_blockIndexSlotsUsed};

					if (this->tailBlock && this->tailBlock->next->Block::template is_empty<explicit_context>()) {
						// We can re-use the block ahead of us, it's empty!
						this->tailBlock = this->tailBlock->next;
						this->tailBlock->Block::template reset_empty<explicit_context>();

						// We'll put the block on the block index (guaranteed to be room since we're conceptually removing the
						// last block from it first -- except instead of removing then adding, we can just overwrite).
						// Note that there must be a valid block index here, since even if allocation failed in the ctor,
						// it would have been re-attempted when adding the first block to the queue; since there is such
						// a block, a block index must have been successfully allocated.
					}
					else {
						// Whatever head value we see here is >= the last value we saw here (relatively),
						// and <= its current value. Since we have the most recent tail, the head must be
						// <= to it.
						auto const head{ this->headIndex.loadRelaxed() };
						assert(!details::circular_less_than<index_t>(currentTailIndex, head));
						if (!details::circular_less_than<index_t>(head, currentTailIndex + BLOCK_SIZE)
								|| (MAX_SUBQUEUE_SIZE != moodycamel::details::const_numeric_max_v<size_t>
									&& (!MAX_SUBQUEUE_SIZE  || MAX_SUBQUEUE_SIZE - BLOCK_SIZE < currentTailIndex - head)))
						{
							// We can't enqueue in another block because there's not enough leeway -- the
							// tail could surpass the head by the time the block fills up! (Or we'll exceed
							// the size limit, if the second part of the condition was true.)
							return {};
						}
						// We're going to need a new block; check that the block index has room
						if (!pr_blockIndexRaw || pr_blockIndexSlotsUsed == pr_blockIndexSize) {
							// Hmm, the circular block index is already full -- we'll need
							// to allocate a new index. Note pr_blockIndexRaw can only be nullptr if
							// the initial allocation failed in the constructor.
							MOODYCAMEL_CONSTEXPR_IF (allocMode == CannotAlloc) { return {}; }
							else { if (!new_block_index(pr_blockIndexSlotsUsed)) { return {} ; } }
						}

						// Insert a new block in the circular linked list
						auto const newBlock { this->parent->XConcurrentQueueAbstract::template requisition_block<allocMode>() };
						if (!newBlock ) { return {}; }
#ifdef MCDBGQ_TRACKMEM
						newBlock->owner = this;
#endif
						newBlock->XConcurrentQueueAbstract::Block::template reset_empty<explicit_context>();

						if (!this->tailBlock) { newBlock->next = newBlock; }
						else {
							newBlock->next = this->tailBlock->next;
							this->tailBlock->next = newBlock;
						}
						this->tailBlock = newBlock;
						++pr_blockIndexSlotsUsed;
					}

					MOODYCAMEL_CONSTEXPR_IF (!MOODYCAMEL_NOEXCEPT_CTOR(T, U, new (static_cast<T*>(nullptr)) T(std::forward<U>(element)))) {
						// The constructor may throw. We want the element not to appear in the queue in
						// that case (without corrupting the queue):
						MOODYCAMEL_TRY {
							new ((*this->tailBlock)[currentTailIndex]) T(std::forward<U>(element));
						}
						MOODYCAMEL_CATCH (...) {
							// Revert change to the current block, but leave the new block available
							// for next time
							pr_blockIndexSlotsUsed = originalBlockIndexSlotsUsed;
							this->tailBlock = startBlock ? startBlock : this->tailBlock;
							MOODYCAMEL_RETHROW;
						}
					} else {
						(void)startBlock;
						(void)originalBlockIndexSlotsUsed;
					}

					// Add block to block index
					auto && entry { blockIndex.loadRelaxed()->entries[pr_blockIndexFront] };
					entry.base = currentTailIndex;
					entry.block = this->tailBlock;
					blockIndex.loadRelaxed()->front.storeRelease(pr_blockIndexFront);
					pr_blockIndexFront = pr_blockIndexFront + 1 & pr_blockIndexSize - 1;

					MOODYCAMEL_CONSTEXPR_IF (!MOODYCAMEL_NOEXCEPT_CTOR(T, U, new (static_cast<T*>(nullptr)) T(std::forward<U>(element)))) {
						this->tailIndex.storeRelease(newTailIndex);
						return true;
					}
				}

				// Enqueue
				new ((*this->tailBlock)[currentTailIndex]) T(std::forward<U>(element));

				this->tailIndex.storeRelease(newTailIndex);
				return true;
			}

			template<typename U>
			constexpr bool dequeue(U & element) {

				if (auto tail{ this->tailIndex.loadRelaxed() }
					, overcommit{ this->dequeueOvercommit.loadRelaxed() };
					details::circular_less_than<index_t>(this->dequeueOptimisticCount.loadRelaxed() - overcommit, tail))
				{
					// Might be something to dequeue, let's give it a try

					// Note that this if is purely for performance purposes in the common case when the queue is
					// empty and the values are eventually consistent -- we may enter here spuriously.

					// Note that whatever the values of overcommit and tail are, they are not going to change (unless we
					// change them) and must be the same value at this point (inside the if) as when the if condition was
					// evaluated.

					// We insert an acquire fence here to synchronize-with the release upon incrementing dequeueOvercommit below.
					// This ensures that whatever the value we got loaded into overcommit, the load of dequeueOptisticCount in
					// the fetch_add below will result in a value at least as recent as that (and therefore at least as large).
					// Note that I believe a compiler (signal) fence here would be sufficient due to the nature of fetch_add (all
					// read-modify-write operations are guaranteed to work on the latest value in the modification order), but
					// unfortunately that can't be shown to be correct using only the C++11 standard.
					// See http://stackoverflow.com/questions/18223161/what-are-the-c11-memory-ordering-guarantees-in-this-corner-case
					std::atomic_thread_fence(std::memory_order_acquire);

					// Increment optimistic counter, then check if it went over the boundary
					auto const myDequeueCount{ this->dequeueOptimisticCount.fetchAndAddRelaxed(1) };

					// Note that since dequeueOvercommit must be <= dequeueOptimisticCount (because dequeueOvercommit is only ever
					// incremented after dequeueOptimisticCount -- this is enforced in the `else` block below), and since we now
					// have a version of dequeueOptimisticCount that is at least as recent as overcommit (due to the release upon
					// incrementing dequeueOvercommit and the acquire above that synchronizes with it), overcommit <= myDequeueCount.
					// However, we can't assert this since both dequeueOptimisticCount and dequeueOvercommit may (independently)
					// overflow; in such a case, though, the logic still holds since the difference between the two is maintained.

					// Note that we reload tail here in case it changed; it will be the same value as before or greater, since
					// this load is sequenced after (happens after) the earlier load above. This is supported by read-read
					// coherency (as defined in the standard), explained here: http://en.cppreference.com/w/cpp/atomic/memory_order
					tail = this->tailIndex.loadAcquire();
					if ((details::likely)(details::circular_less_than<index_t>(myDequeueCount - overcommit, tail))) {
						// Guaranteed to be at least one element to dequeue!

						// Get the index. Note that since there's guaranteed to be at least one element, this
						// will never exceed tail. We need to do an acquire-release fence here since it's possible
						// that whatever condition got us to this point was for an earlier enqueued element (that
						// we already see the memory effects for), but that by the time we increment somebody else
						// has incremented it, and we need to see the memory effects for *that* element, which is
						// in such a case is necessarily visible on the thread that incremented it in the first
						// place with the more current condition (they must have acquired a tail that is at least
						// as recent).
						auto const index{ this->headIndex.fetchAndAddOrdered(1) };

						// Determine which block the element is in

						auto const localBlockIndex { blockIndex.loadAcquire() };
						auto const localBlockIndexHead{ localBlockIndex->front.loadAcquire() };

						// We need to be careful here about subtracting and dividing because of index wrap-around.
						// When an index wraps, we need to preserve the sign of the offset when dividing it by the
						// block size (in order to get a correct signed block count offset in all cases):
						auto const headBase{ localBlockIndex->entries[localBlockIndexHead].base };
						auto const blockBaseIndex{ index & ~static_cast<index_t>(BLOCK_SIZE - 1) };
						using make_signed_t = std::make_signed_t<index_t>;
						auto const offset { static_cast<size_t>(static_cast<make_signed_t>(blockBaseIndex - headBase) / static_cast<make_signed_t>(BLOCK_SIZE)) };
						auto const block { localBlockIndex->entries[localBlockIndexHead + offset & localBlockIndex->size - 1].block };

						// Dequeue
						if (auto && el{ *(*block)[index] };!MOODYCAMEL_NOEXCEPT_ASSIGN(T, T &&, element = std::move(el))) {
							// Make sure the element is still fully dequeued and destroyed even if the assignment
							// throws
							struct Guard {
								Block * block {};
								index_t index {};
								~Guard() {
									(*block)[index]->~T();
									block->XConcurrentQueueAbstract::Block::template set_empty<explicit_context>(index);
								}
							} guard = { block, index };

							element = std::move(el); // NOLINT
						} else {
							element = std::move(el); // NOLINT
							el.~T(); // NOLINT
							block->XConcurrentQueueAbstract::Block::template set_empty<explicit_context>(index);
						}

						return true;
					}
					// Wasn't anything to dequeue after all; make the effective dequeue count eventually consistent
					this->dequeueOvercommit.fetchAndAddRelease(1);		// Release so that the fetch_add on dequeueOptimisticCount is guaranteed to happen before this write
				}
				return {};
			}

			template<AllocationMode allocMode, typename It>
			constexpr bool MOODYCAMEL_NO_TSAN enqueue_bulk(It && items, size_t const count) {

				auto && itemFirst { static_cast<std::decay_t<decltype(items)>>(std::forward<decltype(items)>(items)) };

				// First, we need to make sure we have enough room to enqueue all of the elements;
				// this means pre-allocating blocks and putting them in the block index (but only if
				// all the allocations succeeded).
				auto const startTailIndex{ this->tailIndex.loadRelaxed() };
				auto const startBlock { this->tailBlock };
				auto originalBlockIndexFront{ pr_blockIndexFront };
				auto const originalBlockIndexSlotsUsed{ pr_blockIndexSlotsUsed };

				// Figure out how many blocks we'll need to allocate, and do so
				auto currentTailIndex{ startTailIndex - 1 & ~static_cast<index_t>(BLOCK_SIZE - 1) };
				Block * firstAllocatedBlock {nullptr};

				if (auto blockBaseDiff{ (startTailIndex + count - 1 & ~static_cast<index_t>(BLOCK_SIZE - 1)) - (startTailIndex - 1 & ~static_cast<index_t>(BLOCK_SIZE - 1)) };
					blockBaseDiff > 0)
				{
					// Allocate as many blocks as possible from ahead
					while (blockBaseDiff > 0
						&& this->tailBlock
						&& this->tailBlock->next != firstAllocatedBlock
						&& this->tailBlock->next->Block::template is_empty<explicit_context>())
					{
						blockBaseDiff -= static_cast<index_t>(BLOCK_SIZE);
						currentTailIndex += static_cast<index_t>(BLOCK_SIZE);

						this->tailBlock = this->tailBlock->next;

						if (!firstAllocatedBlock) { firstAllocatedBlock = this->tailBlock; }
						//firstAllocatedBlock = !firstAllocatedBlock ? this->tailBlock : firstAllocatedBlock ;

						auto && entry { blockIndex.loadRelaxed()->entries[pr_blockIndexFront] };
						entry.base = currentTailIndex;
						entry.block = this->tailBlock;
						pr_blockIndexFront = pr_blockIndexFront + 1 & pr_blockIndexSize - 1;
					}

					// Now allocate as many blocks as necessary from the block pool
					while (blockBaseDiff > 0) {

						blockBaseDiff -= static_cast<index_t>(BLOCK_SIZE);
						currentTailIndex += static_cast<index_t>(BLOCK_SIZE);

						auto const head{ this->headIndex.loadRelaxed() };
						assert(!details::circular_less_than<index_t>(currentTailIndex, head));

						if (auto const full{ !details::circular_less_than<index_t>(head, currentTailIndex + BLOCK_SIZE)
								|| (MAX_SUBQUEUE_SIZE != moodycamel::details::const_numeric_max_v<size_t>
								&& (!MAX_SUBQUEUE_SIZE || MAX_SUBQUEUE_SIZE - BLOCK_SIZE < currentTailIndex - head))};
									!pr_blockIndexRaw || pr_blockIndexSlotsUsed == pr_blockIndexSize || full)
						{
#if 0
							MOODYCAMEL_CONSTEXPR_IF (allocMode == CannotAlloc) {
								// Failed to allocate, undo changes (but keep injected blocks)
								pr_blockIndexFront = originalBlockIndexFront;
								pr_blockIndexSlotsUsed = originalBlockIndexSlotsUsed;
								this->tailBlock = startBlock == nullptr ? firstAllocatedBlock : startBlock;
								return {};
							}
							else if (full || !new_block_index(originalBlockIndexSlotsUsed)) {
								// Failed to allocate, undo changes (but keep injected blocks)
								pr_blockIndexFront = originalBlockIndexFront;
								pr_blockIndexSlotsUsed = originalBlockIndexSlotsUsed;
								this->tailBlock = startBlock == nullptr ? firstAllocatedBlock : startBlock;
								return {};
							}
#else
							{
								auto const f{ [this
									,&originalBlockIndexFront
									,&originalBlockIndexSlotsUsed
									,&startBlock
									,&firstAllocatedBlock]()noexcept {
										// Failed to allocate, undo changes (but keep injected blocks)
										pr_blockIndexFront = originalBlockIndexFront;
										pr_blockIndexSlotsUsed = originalBlockIndexSlotsUsed;
										this->tailBlock = startBlock ? startBlock : firstAllocatedBlock;
									}
								};

								MOODYCAMEL_CONSTEXPR_IF(allocMode == CannotAlloc){ f(); return {}; }
								else { if (full || !new_block_index(originalBlockIndexSlotsUsed)) { f(); return {}; } }
							}
#endif
							// pr_blockIndexFront is updated inside new_block_index, so we need to
							// update our fallback value too (since we keep the new index even if we
							// later fail)
							originalBlockIndexFront = originalBlockIndexSlotsUsed;
						}

						// Insert a new block in the circular linked list
						auto const newBlock { this->parent->XConcurrentQueueAbstract::template requisition_block<allocMode>() };
						if (!newBlock) {
							pr_blockIndexFront = originalBlockIndexFront;
							pr_blockIndexSlotsUsed = originalBlockIndexSlotsUsed;
							this->tailBlock = startBlock ? startBlock : firstAllocatedBlock;
							return {};
						}

#ifdef MCDBGQ_TRACKMEM
						newBlock->owner = this;
#endif
						newBlock->XConcurrentQueueAbstract::Block::template set_all_empty<explicit_context>();

						if (!this->tailBlock) { newBlock->next = newBlock; }
						else { newBlock->next = this->tailBlock->next; this->tailBlock->next = newBlock; }
						this->tailBlock = newBlock;

						if (!firstAllocatedBlock) { firstAllocatedBlock = this->tailBlock; }
						//firstAllocatedBlock = !firstAllocatedBlock ? this->tailBlock : firstAllocatedBlock;

						++pr_blockIndexSlotsUsed;

						auto && entry { blockIndex.loadRelaxed()->entries[pr_blockIndexFront] };
						entry.base = currentTailIndex;
						entry.block = this->tailBlock;
						pr_blockIndexFront = pr_blockIndexFront + 1 & pr_blockIndexSize - 1;
					}

					// Excellent, all allocations succeeded. Reset each block's emptiness before we fill them up, and
					// publish the new block index front
					auto block { firstAllocatedBlock };
					while (true) {
						block->XConcurrentQueueAbstract::Block::template reset_empty<explicit_context>();
						if (block == this->tailBlock) { break; }
						block = block->next;
					}

					MOODYCAMEL_CONSTEXPR_IF (MOODYCAMEL_NOEXCEPT_CTOR(T, decltype(*std::forward<decltype(itemFirst)>(itemFirst))
						, new (static_cast<T*>(nullptr)) T(details::deref_noexcept(std::forward<decltype(itemFirst)>(itemFirst)))))
					{ blockIndex.loadRelaxed()->front.storeRelease(pr_blockIndexFront - 1 & pr_blockIndexSize - 1); }
				}

				// Enqueue, one block at a time
				auto const newTailIndex{ startTailIndex + static_cast<index_t>(count) };
				currentTailIndex = startTailIndex;
				auto const endBlock { this->tailBlock };
				this->tailBlock = startBlock;

				assert(startTailIndex & static_cast<index_t>(BLOCK_SIZE - 1) || firstAllocatedBlock || !count);

				if (!(startTailIndex & static_cast<index_t>(BLOCK_SIZE - 1)) && firstAllocatedBlock)
				{ this->tailBlock = firstAllocatedBlock; }

				while (true) {
					auto stopIndex{ (currentTailIndex & ~static_cast<index_t>(BLOCK_SIZE - 1)) + static_cast<index_t>(BLOCK_SIZE) };
					if (details::circular_less_than<index_t>(newTailIndex,stopIndex))  { stopIndex = newTailIndex; }

					MOODYCAMEL_CONSTEXPR_IF (MOODYCAMEL_NOEXCEPT_CTOR(T, decltype(*std::forward<decltype(itemFirst)>(itemFirst))
						, new (static_cast<T*>(nullptr)) T(details::deref_noexcept(std::forward<decltype(itemFirst)>(itemFirst)))))
					{
						while (currentTailIndex != stopIndex) { new ((*this->tailBlock)[currentTailIndex++]) T(*itemFirst++); }
					}else {
						MOODYCAMEL_TRY {
							while (currentTailIndex != stopIndex) {
								// Must use copy constructor even if move constructor is available
								// because we may have to revert if there's an exception.
								// Sorry about the horrible templated next line, but it was the only way
								// to disable moving *at compile time*, which is important because a type
								// may only define a (noexcept) move constructor, and so calls to the
								// cctor will not compile, even if they are in an if branch that will never
								// be executed
								new ((*this->tailBlock)[currentTailIndex]) T(details::nomove_if<!MOODYCAMEL_NOEXCEPT_CTOR(T, decltype(*std::forward<decltype(itemFirst)>(itemFirst))
									, new (static_cast<T*>(nullptr)) T(details::deref_noexcept(std::forward<decltype(itemFirst)>(itemFirst))))>::eval(*std::forward<decltype(itemFirst)>(itemFirst)));
								++currentTailIndex;
								++itemFirst;
							}
						}
						MOODYCAMEL_CATCH (...) {
							// Oh dear, an exception's been thrown -- destroy the elements that
							// were enqueued so far and revert the entire bulk operation (we'll keep
							// any allocated blocks in our linked list for later, though).
							auto const constructedStopIndex{ currentTailIndex };
							auto const lastBlockEnqueued { this->tailBlock };

							pr_blockIndexFront = originalBlockIndexFront;
							pr_blockIndexSlotsUsed = originalBlockIndexSlotsUsed;

							this->tailBlock = startBlock ? startBlock : firstAllocatedBlock;

							if (!details::is_trivially_destructible_v<T>) {
								auto block {startBlock};
								if ( !(startTailIndex & static_cast<index_t>(BLOCK_SIZE - 1)) ) { block = firstAllocatedBlock; }
								currentTailIndex = startTailIndex;
								while (true) {
									stopIndex = (currentTailIndex & ~static_cast<index_t>(BLOCK_SIZE - 1)) + static_cast<index_t>(BLOCK_SIZE);
									if (details::circular_less_than<index_t>(constructedStopIndex, stopIndex)) { stopIndex = constructedStopIndex; }
									while (currentTailIndex != stopIndex) { (*block)[currentTailIndex++]->~T(); }
									if (lastBlockEnqueued == block ) { break; }
									block = block->next;
								}
							}
							MOODYCAMEL_RETHROW;
						}
					}

					if (this->tailBlock == endBlock) {
						assert(currentTailIndex == newTailIndex);
						break;
					}
					this->tailBlock = this->tailBlock->next;
				}

				MOODYCAMEL_CONSTEXPR_IF (!MOODYCAMEL_NOEXCEPT_CTOR(T, decltype(*std::forward<decltype(itemFirst)>(itemFirst))
					, new (static_cast<T*>(nullptr)) T(details::deref_noexcept(std::forward<decltype(itemFirst)>(itemFirst)))))
				{
					if (firstAllocatedBlock)
					{ blockIndex.loadRelaxed()->front.storeRelease(pr_blockIndexFront - 1 & pr_blockIndexSize - 1); }
				}

				this->tailIndex.storeRelease(newTailIndex);
				return true;
			}

			template<typename It>
			constexpr size_t dequeue_bulk(It && items, size_t const max) {

				auto && itemFirst{ static_cast<std::decay_t<decltype(items)>>(std::forward<decltype(items)>(items)) };

				auto tail{ this->tailIndex.loadRelaxed() };
				auto const overcommit{ this->dequeueOvercommit.loadRelaxed() };
				auto desiredCount { static_cast<size_t>(tail - (this->dequeueOptimisticCount.loadRelaxed() - overcommit)) };

				if (details::circular_less_than<size_t>(0, desiredCount)) {

					desiredCount = desiredCount < max ? desiredCount : max;

					std::atomic_thread_fence(std::memory_order_acquire);

					auto const myDequeueCount{ this->dequeueOptimisticCount.fetchAndAddRelaxed(desiredCount) };

					tail = this->tailIndex.loadAcquire();
					auto actualCount { static_cast<size_t>(tail - (myDequeueCount - overcommit)) };
					if (details::circular_less_than<size_t>(0, actualCount)) {

						actualCount = desiredCount < actualCount ? desiredCount : actualCount;

						if (actualCount < desiredCount)
						{ this->dequeueOvercommit.fetchAndAddRelease(desiredCount - actualCount); }

						// Get the first index. Note that since there's guaranteed to be at least actualCount elements, this
						// will never exceed tail.
						auto const firstIndex{ this->headIndex.fetchAndAddOrdered(actualCount)};

						// Determine which block the first element is in
						auto const localBlockIndex { blockIndex.loadAcquire() };
						auto const localBlockIndexHead{ localBlockIndex->front.loadAcquire() };

						auto const headBase{ localBlockIndex->entries[localBlockIndexHead].base};
						auto const firstBlockBaseIndex{ firstIndex & ~static_cast<index_t>(BLOCK_SIZE - 1) };

						using make_signed_t = std::make_signed_t<index_t>;
						auto const offset { static_cast<size_t>(static_cast<make_signed_t>(firstBlockBaseIndex - headBase)
							/ static_cast<make_signed_t>(BLOCK_SIZE))
						};

						auto indexIndex{ localBlockIndexHead + offset & localBlockIndex->size - 1 };

						// Iterate the blocks and dequeue
						auto index{ firstIndex };
						do {
							auto firstIndexInBlock{ index };
							auto endIndex{ (index & ~static_cast<index_t>(BLOCK_SIZE - 1)) + static_cast<index_t>(BLOCK_SIZE) };
							endIndex = details::circular_less_than<index_t>(firstIndex + static_cast<index_t>(actualCount), endIndex)
								? firstIndex + static_cast<index_t>(actualCount)
								: endIndex;

							auto block { localBlockIndex->entries[indexIndex].block };

							if (MOODYCAMEL_NOEXCEPT_ASSIGN(T, T &&, details::deref_noexcept(std::forward<decltype(itemFirst)>(itemFirst)) = std::move(*(*block)[index]))) {
								while (index != endIndex) {
									auto && el{ *(*block)[index] };
									*itemFirst++ = std::move(el);
									el.~T();
									++index;
								}
							} else {
								MOODYCAMEL_TRY {
									while (index != endIndex) {
										auto && el{ *(*block)[index] };
										*itemFirst = std::move(el);
										++itemFirst;
										el.~T();
										++index;
									}
								}
								MOODYCAMEL_CATCH (...) {
									// It's too late to revert the dequeue, but we can make sure that all
									// the dequeued objects are properly destroyed and the block index
									// (and empty count) are properly updated before we propagate the exception
									do {
										block = localBlockIndex->entries[indexIndex].block; (void)block;
										while (index != endIndex) { (*block)[index++]->~T(); }
										block->XConcurrentQueueAbstract::Block::template set_many_empty<explicit_context>(firstIndexInBlock, static_cast<size_t>(endIndex - firstIndexInBlock));
										indexIndex = indexIndex + 1 & localBlockIndex->size - 1;

										firstIndexInBlock = index;
										endIndex = (index & ~static_cast<index_t>(BLOCK_SIZE - 1)) + static_cast<index_t>(BLOCK_SIZE);
										endIndex = details::circular_less_than<index_t>(firstIndex + static_cast<index_t>(actualCount), endIndex) ? firstIndex + static_cast<index_t>(actualCount) : endIndex;
									} while (index != firstIndex + actualCount);

									MOODYCAMEL_RETHROW;
								}
							}
							block->XConcurrentQueueAbstract::Block::template set_many_empty<explicit_context>(firstIndexInBlock, static_cast<size_t>(endIndex - firstIndexInBlock));
							indexIndex = indexIndex + 1 & localBlockIndex->size - 1;
						} while (index != firstIndex + actualCount);

						return actualCount;
					}
					// Wasn't anything to dequeue after all; make the effective dequeue count eventually consistent
					this->dequeueOvercommit.fetchAndAddRelease(desiredCount);
				}

				return {};
			}

		private:

			struct BlockIndexEntry {
				index_t base {};
				Block* block {};
			};

			struct BlockIndexHeader {
				size_t size {};
				XAtomicInteger<size_t> front {};	// Current slot (not next, like pr_blockIndexFront)
				BlockIndexEntry * entries {};
				void * prev{};
			};

			constexpr bool new_block_index(size_t const numberOfFilledSlotsToExpose) {

				auto const prevBlockSizeMask{ pr_blockIndexSize - 1};

				// Create the new block
				pr_blockIndexSize <<= 1;
				auto const newRawPtr {
					static_cast<char*>((Traits::malloc)(sizeof(BlockIndexHeader)
						+ std::alignment_of_v<BlockIndexEntry> - 1
						+ sizeof(BlockIndexEntry) * pr_blockIndexSize))
				};
				if (!newRawPtr) {
					pr_blockIndexSize >>= 1;		// Reset to allow graceful retry
					return {};
				}

				auto const newBlockIndexEntries{
					reinterpret_cast<BlockIndexEntry*>(details::align_for<BlockIndexEntry>(newRawPtr + sizeof(BlockIndexHeader)))
				};

				// Copy in all the old indices, if any
				size_t j {};
				if (pr_blockIndexSlotsUsed) {
					auto i{ pr_blockIndexFront - pr_blockIndexSlotsUsed & prevBlockSizeMask };
					do {
						newBlockIndexEntries[j++] = pr_blockIndexEntries[i];
						i = i + 1 & prevBlockSizeMask;
					} while (i != pr_blockIndexFront);
				}

				// Update everything
				auto const header { new (newRawPtr) BlockIndexHeader };
				header->size = pr_blockIndexSize;
				header->front.storeRelaxed(numberOfFilledSlotsToExpose - 1);
				header->entries = newBlockIndexEntries;
				header->prev = pr_blockIndexRaw;		// we link the new block to the old one so we can free it later

				pr_blockIndexFront = j;
				pr_blockIndexEntries = newBlockIndexEntries;
				pr_blockIndexRaw = newRawPtr;
				blockIndex.storeRelease(header);

				return true;
			}

			XAtomicPointer<BlockIndexHeader> blockIndex {};

			// To be used by producer only -- consumer must use the ones in referenced by blockIndex
			size_t pr_blockIndexSlotsUsed {}
			,pr_blockIndexSize {}
			,pr_blockIndexFront {};		// Next slot (not current)
			BlockIndexEntry * pr_blockIndexEntries {};
			void * pr_blockIndexRaw{};

#ifdef MOODYCAMEL_QUEUE_INTERNAL_DEBUG
		public:
			ExplicitProducer * nextExplicitProducer {};
#endif
			friend struct MemStats;
		};

		//////////////////////////////////
		// Implicit queue
		//////////////////////////////////
		struct ImplicitProducer : ProducerBase {
			ImplicitProducer(XConcurrentQueueAbstract * const parent_) :
				ProducerBase(parent_, {}),
				nextBlockIndexCapacity(IMPLICIT_INITIAL_INDEX_SIZE)
			{ new_block_index(); }

			~ImplicitProducer() override {
				// Note that since we're in the destructor we can assume that all enqueue/dequeue operations
				// completed already; this means that all undequeued elements are placed contiguously across
				// contiguous blocks, and that only the first and last remaining blocks can be only partially
				// empty (all other remaining blocks must be completely full).

#ifdef MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED
				// Unregister ourselves for thread termination notification
				if (!this->inactive.loadRelaxed()) {
					details::ThreadExitNotifier::unsubscribe(&threadExitListener);
				}
#endif

				// Destroy all remaining elements!
				auto const tail{ this->tailIndex.loadRelaxed() };
				auto index{ this->headIndex.loadRelaxed() };
				Block * block {};
				assert(index == tail || details::circular_less_than(index, tail));
				auto const forceFreeLastBlock{ index != tail };		// If we enter the loop, then the last (tail) block will not be freed
				while (index != tail) {
					if ( index & static_cast<index_t>(BLOCK_SIZE - 1) || !block ) {
						if (block) { this->parent->add_block_to_free_list(block); } // Free the old block
						block = get_block_index_entry_for_index(index)->value.loadRelaxed();
					}
					(*block)[index]->~T();
					++index;
				}
				// Even if the queue is empty, there's still one block that's not on the free list
				// (unless the head index reached the end of it, in which case the tail will be poised
				// to create a new block).
				if ( this->tailBlock
					&& (forceFreeLastBlock || tail & static_cast<index_t>(BLOCK_SIZE - 1) ) )
				{ this->parent->add_block_to_free_list(this->tailBlock); }

				// Destroy block index
				auto localBlockIndex { blockIndex.loadRelaxed() };
				if (localBlockIndex) {
					for (size_t i {}; i != localBlockIndex->capacity; ++i)
					{ localBlockIndex->index[i]->~BlockIndexEntry(); }
					do {
						auto const prev{ localBlockIndex->prev };
						localBlockIndex->~BlockIndexHeader();
						(Traits::free)(localBlockIndex);
						localBlockIndex = prev;
					} while (localBlockIndex);
				}
			}

			template<AllocationMode allocMode, typename U>
			constexpr bool enqueue(U && element) {
				auto const currentTailIndex{ this->tailIndex.loadRelaxed() };
				auto const newTailIndex { 1 + currentTailIndex };
				if (! (currentTailIndex & static_cast<index_t>(BLOCK_SIZE - 1)) ) {
					// We reached the end of a block, start a new one
					auto const head{ this->headIndex.loadRelaxed() };
					assert(!details::circular_less_than<index_t>(currentTailIndex, head));

					if (!details::circular_less_than<index_t>(head, currentTailIndex + BLOCK_SIZE)
						|| (MAX_SUBQUEUE_SIZE != moodycamel::details::const_numeric_max_v<size_t>
							&& (!MAX_SUBQUEUE_SIZE || MAX_SUBQUEUE_SIZE - BLOCK_SIZE < currentTailIndex - head)))
					{ return {}; }
#ifdef MCDBGQ_NOLOCKFREE_IMPLICITPRODBLOCKINDEX
					debug::DebugLock lock {mutex};
#endif
					// Find out where we'll be inserting this block in the block index
					BlockIndexEntry * idxEntry {};
					if (!insert_block_index_entry<allocMode>(idxEntry, currentTailIndex)) { return {}; }

					// Get ahold of a new block
					auto const newBlock{ this->parent->XConcurrentQueueAbstract::template requisition_block<allocMode>() };
					if (!newBlock) {
						rewind_block_index_tail();
						idxEntry->value.storeRelaxed({});
						return {};
					}
#ifdef MCDBGQ_TRACKMEM
					newBlock->owner = this;
#endif
					newBlock->XConcurrentQueueAbstract::Block::template reset_empty<implicit_context>();

					MOODYCAMEL_CONSTEXPR_IF (!MOODYCAMEL_NOEXCEPT_CTOR(T, U, new (static_cast<T*>(nullptr)) T(std::forward<U>(element)))) {
						// May throw, try to insert now before we publish the fact that we have this new block
						MOODYCAMEL_TRY {
							new ((*newBlock)[currentTailIndex]) T(std::forward<U>(element));
						}
						MOODYCAMEL_CATCH (...) {
							rewind_block_index_tail();
							idxEntry->value.storeRelaxed({});
							this->parent->add_block_to_free_list(newBlock);
							MOODYCAMEL_RETHROW;
						}
					}

					// Insert the new block into the index
					idxEntry->value.storeRelaxed(newBlock);

					this->tailBlock = newBlock;

					MOODYCAMEL_CONSTEXPR_IF (!MOODYCAMEL_NOEXCEPT_CTOR(T, U, new (static_cast<T*>(nullptr)) T(std::forward<U>(element)))) {
						this->tailIndex.storeRelease(newTailIndex);
						return true;
					}
				}

				// Enqueue
				new ((*this->tailBlock)[currentTailIndex]) T(std::forward<U>(element));

				this->tailIndex.storeRelease(newTailIndex);
				return true;
			}

			template<typename U>
			constexpr bool dequeue(U & element) {
				// See ExplicitProducer::dequeue for rationale and explanation
				auto tail{ this->tailIndex.loadRelaxed() };
				auto const overcommit { this->dequeueOvercommit.loadRelaxed() };

				if (details::circular_less_than<index_t>(this->dequeueOptimisticCount.loadRelaxed() - overcommit, tail)) {
					std::atomic_thread_fence(std::memory_order_acquire);

					auto const myDequeueCount{ this->dequeueOptimisticCount.fetchAndAddRelaxed(1) };
					tail = this->tailIndex.loadAcquire();
					if ((details::likely)(details::circular_less_than<index_t>(myDequeueCount - overcommit, tail))) {
						auto const index{ this->headIndex.fetchAndAddOrdered(1) };

						// Determine which block the element is in
						auto const entry { get_block_index_entry_for_index(index) };

						// Dequeue
						auto const block { entry->value.loadRelaxed() };

						if (auto && el { *(*block)[index] };!MOODYCAMEL_NOEXCEPT_ASSIGN(T, T&&, element = std::move(el))) {
#ifdef MCDBGQ_NOLOCKFREE_IMPLICITPRODBLOCKINDEX
							// Note: Acquiring the mutex with every dequeue instead of only when a block
							// is released is very sub-optimal, but it is, after all, purely debug code.
							debug::DebugLock lock { producer->mutex };
#endif
							struct Guard {
								Block * block {};
								index_t index{};
								BlockIndexEntry * entry {};
								XConcurrentQueueAbstract * parent {};

								~Guard() {
									(*block)[index]->~T();
									if (block->XConcurrentQueueAbstract::Block::template set_empty<implicit_context>(index)) {
										entry->value.storeRelaxed({});
										parent->add_block_to_free_list(block);
									}
								}
							} guard = { block, index, entry, this->parent };

							element = std::move(el); // NOLINT
						}else {
							element = std::move(el); // NOLINT
							el.~T(); // NOLINT

							if (block->XConcurrentQueueAbstract::Block::template set_empty<implicit_context>(index)) {
								{
#ifdef MCDBGQ_NOLOCKFREE_IMPLICITPRODBLOCKINDEX
									debug::DebugLock lock { mutex };
#endif
									// Add the block back into the global free pool (and remove from block index)
									entry->value.storeRelaxed({});
								}
								this->parent->add_block_to_free_list(block);		// releases the above store
							}
						}

						return true;
					}
					this->dequeueOvercommit.fetchAndAddRelease(1);
				}

				return {};
			}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4706)  // assignment within conditional expression
#endif

			template<AllocationMode allocMode, typename It>
			constexpr bool enqueue_bulk(It && items, size_t const count) {

				auto && itemFirst{ static_cast<std::decay_t<decltype(items)>>(std::forward<decltype(items)>(items)) };

				// First, we need to make sure we have enough room to enqueue all of the elements;
				// this means pre-allocating blocks and putting them in the block index (but only if
				// all the allocations succeeded).

				// Note that the tailBlock we start off with may not be owned by us any more;
				// this happens if it was filled up exactly to the top (setting tailIndex to
				// the first index of the next block which is not yet allocated), then dequeued
				// completely (putting it on the free list) before we enqueue again.

				auto const startTailIndex{ this->tailIndex.loadRelaxed() };
				auto const startBlock { this->tailBlock };
				Block * firstAllocatedBlock { nullptr };
				auto endBlock { this->tailBlock };

				// Figure out how many blocks we'll need to allocate, and do so
				auto blockBaseDiff{ (startTailIndex + count - 1 & ~static_cast<index_t>(BLOCK_SIZE - 1)) - (startTailIndex - 1 & ~static_cast<index_t>(BLOCK_SIZE - 1)) };
				auto currentTailIndex{ startTailIndex - 1 & ~static_cast<index_t>(BLOCK_SIZE - 1) };
				if (blockBaseDiff > 0) {
#ifdef MCDBGQ_NOLOCKFREE_IMPLICITPRODBLOCKINDEX
					debug::DebugLock lock { mutex };
#endif
					do {
						blockBaseDiff -= static_cast<index_t>(BLOCK_SIZE);
						currentTailIndex += static_cast<index_t>(BLOCK_SIZE);

						// Find out where we'll be inserting this block in the block index
						BlockIndexEntry * idxEntry {};  // initialization here unnecessary but compiler can't always tell
						Block * newBlock;
						bool indexInserted {};
						auto const head{ this->headIndex.loadRelaxed() };
						assert(!details::circular_less_than<index_t>(currentTailIndex, head));

						if (auto const full{ !details::circular_less_than<index_t>(head, currentTailIndex + BLOCK_SIZE)
								|| (MAX_SUBQUEUE_SIZE != moodycamel::details::const_numeric_max_v<size_t>
									 && (!MAX_SUBQUEUE_SIZE || MAX_SUBQUEUE_SIZE - BLOCK_SIZE < currentTailIndex - head))};
							full
							|| !( (indexInserted = insert_block_index_entry<allocMode>(idxEntry, currentTailIndex)) )
							|| !((newBlock = this->parent->XConcurrentQueueAbstract::template requisition_block<allocMode>())))
						{
							// Index allocation or block allocation failed; revert any other allocations
							// and index insertions done so far for this operation
							if (indexInserted) {
								rewind_block_index_tail();
								idxEntry->value.storeRelaxed({});
							}

							currentTailIndex = startTailIndex - 1 & ~static_cast<index_t>(BLOCK_SIZE - 1);

							for (auto block{firstAllocatedBlock}; block; block = block->next) {
								currentTailIndex += static_cast<index_t>(BLOCK_SIZE);
								idxEntry = get_block_index_entry_for_index(currentTailIndex);
								idxEntry->value.storeRelaxed({});
								rewind_block_index_tail();
							}
							this->parent->add_blocks_to_free_list(firstAllocatedBlock);
							this->tailBlock = startBlock;

							return {};
						}

#ifdef MCDBGQ_TRACKMEM
						newBlock->owner = this;
#endif
						newBlock->XConcurrentQueueAbstract::Block::template reset_empty<implicit_context>();
						newBlock->next = {};

						// Insert the new block into the index
						idxEntry->value.storeRelaxed(newBlock);

						// Store the chain of blocks so that we can undo if later allocations fail,
						// and so that we can find the blocks when we do the actual enqueueing
						if (startTailIndex & static_cast<index_t>(BLOCK_SIZE - 1) || firstAllocatedBlock) {
							assert(this->tailBlock);
							this->tailBlock->next = newBlock;
						}
						this->tailBlock = newBlock;
						endBlock = newBlock;
						firstAllocatedBlock = firstAllocatedBlock ? firstAllocatedBlock : newBlock;
					} while (blockBaseDiff > 0);
				}

				// Enqueue, one block at a time
				auto const newTailIndex { startTailIndex + static_cast<index_t>(count) };
				currentTailIndex = startTailIndex;
				this->tailBlock = startBlock;
				assert( startTailIndex & static_cast<index_t>(BLOCK_SIZE - 1) || firstAllocatedBlock || !count );

				if (!(startTailIndex & static_cast<index_t>(BLOCK_SIZE - 1)) && firstAllocatedBlock )
				{ this->tailBlock = firstAllocatedBlock; }

				while (true) {
					auto stopIndex{ (currentTailIndex & ~static_cast<index_t>(BLOCK_SIZE - 1)) + static_cast<index_t>(BLOCK_SIZE) };
					if (details::circular_less_than<index_t>(newTailIndex, stopIndex)) { stopIndex = newTailIndex; }

					MOODYCAMEL_CONSTEXPR_IF (MOODYCAMEL_NOEXCEPT_CTOR(T, decltype(*itemFirst)
						, new (static_cast<T*>(nullptr)) T(details::deref_noexcept(std::forward<decltype(itemFirst)>(itemFirst)))))
					{
						while (currentTailIndex != stopIndex)
						{ new ((*this->tailBlock)[currentTailIndex++]) T(*itemFirst++); }
					} else {
						MOODYCAMEL_TRY {
							while (currentTailIndex != stopIndex) {
								new ((*this->tailBlock)[currentTailIndex]) T(details::nomove_if<!MOODYCAMEL_NOEXCEPT_CTOR(T, decltype(*itemFirst)
									, new (static_cast<T*>(nullptr)) T(details::deref_noexcept(std::forward<decltype(itemFirst)>(itemFirst))))>::eval(*itemFirst));
								++currentTailIndex;
								++itemFirst;
							}
						}
						MOODYCAMEL_CATCH (...) {
							auto const constructedStopIndex{ currentTailIndex };
							auto const lastBlockEnqueued { this->tailBlock };

							if (!details::is_trivially_destructible_v<T>) {

								auto block{startBlock};

								if (!(startTailIndex & static_cast<index_t>(BLOCK_SIZE - 1)))
								{ block = firstAllocatedBlock; (void)block; }

								currentTailIndex = startTailIndex;

								while (true) {
									stopIndex = (currentTailIndex & ~static_cast<index_t>(BLOCK_SIZE - 1)) + static_cast<index_t>(BLOCK_SIZE);

									if (details::circular_less_than<index_t>(constructedStopIndex, stopIndex))
									{ stopIndex = constructedStopIndex; }

									while (currentTailIndex != stopIndex)
									{ (*block)[currentTailIndex++]->~T(); }

									if (block == lastBlockEnqueued) { break; }
									block = block->next;
								}
							}

							currentTailIndex = startTailIndex - 1 & ~static_cast<index_t>(BLOCK_SIZE - 1);(void)currentTailIndex;

							for (auto b{ firstAllocatedBlock }; b; b = b->next) {
								currentTailIndex += static_cast<index_t>(BLOCK_SIZE);
								auto const idxEntry { get_block_index_entry_for_index(currentTailIndex) };
								idxEntry->value.storeRelaxed({});
								rewind_block_index_tail();
							}

							this->parent->add_blocks_to_free_list(firstAllocatedBlock);
							this->tailBlock = startBlock;
							MOODYCAMEL_RETHROW;
						}
					}

					if (this->tailBlock == endBlock) {
						assert(currentTailIndex == newTailIndex);
						break;
					}
					this->tailBlock = this->tailBlock->next;
				}
				this->tailIndex.storeRelease(newTailIndex);
				return true;
			}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

			template<typename It>
			constexpr size_t dequeue_bulk(It && items, size_t const max) {

				auto && itemFirst { static_cast<std::decay_t<decltype(items)>>(std::forward<decltype(items)>(items)) };

				auto tail{ this->tailIndex.loadRelaxed() };
				auto const overcommit{ this->dequeueOvercommit.loadRelaxed() };
				auto desiredCount{ static_cast<size_t>(tail - (this->dequeueOptimisticCount.loadRelaxed() - overcommit)) };

				if (details::circular_less_than<size_t>(0, desiredCount)) {
					desiredCount = desiredCount < max ? desiredCount : max;
					std::atomic_thread_fence(std::memory_order_acquire);

					auto const myDequeueCount{ this->dequeueOptimisticCount.fetchAndAddRelaxed(desiredCount) };

					tail = this->tailIndex.loadAcquire();
					auto actualCount { static_cast<size_t>(tail - (myDequeueCount - overcommit)) };
					if (details::circular_less_than<size_t>(0, actualCount)) {

						actualCount = desiredCount < actualCount ? desiredCount : actualCount;

						if (actualCount < desiredCount)
						{ this->dequeueOvercommit.fetchAndAddRelease(desiredCount - actualCount); }

						// Get the first index. Note that since there's guaranteed to be at least actualCount elements, this
						// will never exceed tail.
						auto const firstIndex{ this->headIndex.fetchAndAddOrdered(actualCount) };

						// Iterate the blocks and dequeue
						auto index{ firstIndex };
						BlockIndexHeader * localBlockIndex {};
						auto indexIndex{ get_block_index_index_for_index(index, localBlockIndex) };

						do {
							auto blockStartIndex{ index };
							auto endIndex{ (index & ~static_cast<index_t>(BLOCK_SIZE - 1)) + static_cast<index_t>(BLOCK_SIZE) };
#if 0
							endIndex = details::circular_less_than<index_t>(firstIndex + static_cast<index_t>(actualCount), endIndex)
								? firstIndex + static_cast<index_t>(actualCount)
								: endIndex;
#else
							if ( details::circular_less_than<index_t>(firstIndex + static_cast<index_t>(actualCount), endIndex) )
							{ endIndex = firstIndex + static_cast<index_t>(actualCount); }
#endif
							auto entry { localBlockIndex->index[indexIndex] };
							auto block { entry->value.loadRelaxed() };
							if (MOODYCAMEL_NOEXCEPT_ASSIGN(T, T&&, details::deref_noexcept(std::forward<decltype(itemFirst)>(itemFirst)) = std::move(*(*block)[index]))) {
								while (index != endIndex) {
									auto && el{ *(*block)[index] };
									*itemFirst++ = std::move(el);
									el.~T();
									++index;
								}
							}else {
								MOODYCAMEL_TRY {
									while (index != endIndex) {
										auto && el{*(*block)[index]};
										*itemFirst = std::move(el);
										++itemFirst;
										el.~T();
										++index;
									}
								}
								MOODYCAMEL_CATCH (...) {
									do {
										entry = localBlockIndex->index[indexIndex];
										block = entry->value.loadRelaxed();(void)block;
										while (index != endIndex) { (*block)[index++]->~T(); }

										if (block->XConcurrentQueueAbstract::Block::template set_many_empty<implicit_context>(blockStartIndex, static_cast<size_t>(endIndex - blockStartIndex))) {
#ifdef MCDBGQ_NOLOCKFREE_IMPLICITPRODBLOCKINDEX
											debug::DebugLock lock { mutex };
#endif
											entry->value.storeRelaxed({});
											this->parent->add_block_to_free_list(block);
										}
										indexIndex = indexIndex + 1 & localBlockIndex->capacity - 1;

										blockStartIndex = index;
										endIndex = (index & ~static_cast<index_t>(BLOCK_SIZE - 1)) + static_cast<index_t>(BLOCK_SIZE);
										endIndex = details::circular_less_than<index_t>(firstIndex + static_cast<index_t>(actualCount), endIndex) ? firstIndex + static_cast<index_t>(actualCount) : endIndex;
									} while (index != firstIndex + actualCount);

									MOODYCAMEL_RETHROW;
								}
							}
							if (block->XConcurrentQueueAbstract::Block::template set_many_empty<implicit_context>(blockStartIndex, static_cast<size_t>(endIndex - blockStartIndex))) {
								{
#ifdef MCDBGQ_NOLOCKFREE_IMPLICITPRODBLOCKINDEX
									debug::DebugLock lock {mutex};
#endif
									// Note that the set_many_empty above did a release, meaning that anybody who acquires the block
									// we're about to free can use it safely since our writes (and reads!) will have happened-before then.
									entry->value.storeRelaxed({});
								}
								this->parent->add_block_to_free_list(block);		// releases the above store
							}
							indexIndex = indexIndex + 1 & localBlockIndex->capacity - 1;
						} while (index != firstIndex + actualCount);

						return actualCount;
					}

					this->dequeueOvercommit.fetchAndAddRelease(desiredCount);
				}

				return {};
			}

		private:
			// The block size must be > 1, so any number with the low bit set is an invalid block base index
			static constexpr index_t INVALID_BLOCK_BASE {1};

			struct BlockIndexEntry {
				XAtomicInteger<index_t> key {};
				XAtomicPointer<Block> value {};
			};

			struct BlockIndexHeader {
				size_t capacity{};
				XAtomicInteger<size_t> tail {};
				BlockIndexEntry * entries{};
				BlockIndexEntry ** index{};
				BlockIndexHeader * prev{};
			};

			template<AllocationMode allocMode>
			constexpr bool insert_block_index_entry(BlockIndexEntry * & idxEntry, index_t const blockStartIndex) {
				auto localBlockIndex { blockIndex.loadRelaxed() };		// We're the only writer thread, relaxed is OK
				if (!localBlockIndex ) { return {}; }  // this can happen if new_block_index failed in the constructor
				auto newTail{ localBlockIndex->tail.loadRelaxed() + 1 & localBlockIndex->capacity - 1 };
				idxEntry = localBlockIndex->index[newTail];
				if (idxEntry->key.loadRelaxed() == INVALID_BLOCK_BASE
					|| !idxEntry->value.loadRelaxed() )
				{
					idxEntry->key.storeRelaxed(blockStartIndex);
					localBlockIndex->tail.storeRelease(newTail);
					return true;
				}

				// No room in the old block index, try to allocate another one!
				MOODYCAMEL_CONSTEXPR_IF (allocMode == CannotAlloc) { return {}; }
				else {
					if (!new_block_index()) { return {}; }
					localBlockIndex = blockIndex.loadRelaxed();
					newTail = localBlockIndex->tail.loadRelaxed() + 1 & localBlockIndex->capacity - 1;
					idxEntry = localBlockIndex->index[newTail];
					assert(idxEntry->key.loadRelaxed() == INVALID_BLOCK_BASE);
					idxEntry->key.storeRelaxed(blockStartIndex);
					localBlockIndex->tail.storeRelease(newTail);
					return true;
				}
			}

			constexpr void rewind_block_index_tail() {
				auto const localBlockIndex { blockIndex.loadRelaxed() };
				localBlockIndex->tail.storeRelaxed(localBlockIndex->tail.loadRelaxed() - 1 & localBlockIndex->capacity - 1);
			}

			constexpr BlockIndexEntry * get_block_index_entry_for_index(index_t const index) const {
				BlockIndexHeader * localBlockIndex{};
				auto const idx{ get_block_index_index_for_index(index, localBlockIndex)};
				return localBlockIndex->index[idx];
			}

			constexpr size_t get_block_index_index_for_index(index_t index, BlockIndexHeader *& localBlockIndex) const {
#ifdef MCDBGQ_NOLOCKFREE_IMPLICITPRODBLOCKINDEX
				debug::DebugLock lock {mutex};
#endif
				index &= ~static_cast<index_t>(BLOCK_SIZE - 1);
				localBlockIndex = blockIndex.loadAcquire();
				auto const tail{ localBlockIndex->tail.loadAcquire() };
				auto const tailBase{ localBlockIndex->index[tail]->key.loadRelaxed() };
				assert(tailBase != INVALID_BLOCK_BASE);

				// Note: Must use division instead of shift because the index may wrap around, causing a negative
				// offset, whose negativity we want to preserve
				using make_signed_t = std::make_signed_t<index_t>;
				auto const offset{ static_cast<size_t>(static_cast<make_signed_t>(index - tailBase)
					/ static_cast<make_signed_t>(BLOCK_SIZE))
				};
				auto const idx{ tail + offset & localBlockIndex->capacity - 1 };
				assert(localBlockIndex->index[idx]->key.loadRelaxed() == index
					&& localBlockIndex->index[idx]->value.loadRelaxed());
				return idx;
			}

			constexpr bool new_block_index() {
				auto const prev{ blockIndex.loadRelaxed() };
				auto const prevCapacity{ prev ? prev->capacity : 0 };
				auto const entryCount{ !prev ? nextBlockIndexCapacity : prevCapacity };
				auto const raw { static_cast<char*>((Traits::malloc)(
					sizeof(BlockIndexHeader) +
					std::alignment_of_v<BlockIndexEntry> - 1 + sizeof(BlockIndexEntry) * entryCount +
					std::alignment_of_v<BlockIndexEntry*> - 1 + sizeof(BlockIndexEntry*) * nextBlockIndexCapacity))
				};
				if (!raw) { return {}; }

				auto const header{ new (raw) BlockIndexHeader};
				auto const entries{ reinterpret_cast<BlockIndexEntry*>(details::align_for<BlockIndexEntry>(raw + sizeof(BlockIndexHeader))) };
				auto const index {
					reinterpret_cast<BlockIndexEntry**>(details::align_for<BlockIndexEntry*>(reinterpret_cast<char*>(entries)
						+ sizeof(BlockIndexEntry) * entryCount))
				};

				if (prev) {
					auto const prevTail{ prev->tail.loadRelaxed() };
					auto prevPos{ prevTail };
					size_t i {};
					do {
						prevPos = prevPos + 1 & prev->capacity - 1;
						index[i++] = prev->index[prevPos];
					} while (prevTail != prevPos);
					assert(i == prevCapacity);
				}

				for (size_t i {}; i != entryCount; ++i) {
					new (entries + i) BlockIndexEntry;
					entries[i].key.storeRelaxed(INVALID_BLOCK_BASE);
					index[prevCapacity + i] = entries + i;
				}

				header->prev = prev;
				header->entries = entries;
				header->index = index;
				header->capacity = nextBlockIndexCapacity;
				header->tail.storeRelaxed(prevCapacity - 1 & nextBlockIndexCapacity - 1);

				blockIndex.storeRelease(header);

				nextBlockIndexCapacity <<= 1;

				return true;
			}

			size_t nextBlockIndexCapacity {};
			XAtomicPointer<BlockIndexHeader> blockIndex {};

#ifdef MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED
		public:
			details::ThreadExitListener threadExitListener{};
#endif

#ifdef MOODYCAMEL_QUEUE_INTERNAL_DEBUG
		public:
			ImplicitProducer * nextImplicitProducer{};
		private:
#endif

#ifdef MCDBGQ_NOLOCKFREE_IMPLICITPRODBLOCKINDEX
			mutable debug::DebugMutex mutex{};
#endif
#ifdef MCDBGQ_TRACKMEM
			friend struct MemStats;
#endif
		};

		//////////////////////////////////
		// Implicit producer hash
		//////////////////////////////////
		struct ImplicitProducerKVP {
			std::atomic<details::thread_id_t> key{};
			ImplicitProducer * value {};		// No need for atomicity since it's only read by the thread that sets it in the first place

			constexpr ImplicitProducerKVP() = default;

			ImplicitProducerKVP(ImplicitProducerKVP && other) MOODYCAMEL_NOEXCEPT {
				key.store(other.key.load(std::memory_order_relaxed), std::memory_order_relaxed);
				value = other.value;
			}

			ImplicitProducerKVP& operator=(ImplicitProducerKVP && other) MOODYCAMEL_NOEXCEPT
			{ swap(other); return *this; }

			constexpr void swap(ImplicitProducerKVP & other) MOODYCAMEL_NOEXCEPT {
				if (this != std::addressof(other)) {
					details::swap_relaxed(key, other.key);
					std::swap(value, other.value);
				}
			}
		};

		struct ImplicitProducerHash {
			size_t capacity{};
			ImplicitProducerKVP * entries{};
			ImplicitProducerHash * prev{};
		};

		XAtomicPointer<ProducerBase> producerListTail {};
		XAtomicInteger<std::uint32_t> producerCount{};

		XAtomicInteger<size_t> initialBlockPoolIndex{};
		Block * initialBlockPool {};
		size_t initialBlockPoolSize {};

#ifndef MCDBGQ_USEDEBUGFREELIST
		FreeList<Block> freeList{};
#else
		debug::DebugFreeList<Block> freeList{};
#endif

		XAtomicPointer<ImplicitProducerHash> implicitProducerHash{};
		XAtomicInteger<size_t> implicitProducerHashCount{}; // Number of slots logically used
		ImplicitProducerHash initialImplicitProducerHash{};
		std::array<ImplicitProducerKVP, INITIAL_IMPLICIT_PRODUCER_HASH_SIZE> initialImplicitProducerHashEntries{};
		std::atomic_flag implicitProducerHashResizeInProgress{};

		XAtomicInteger<std::uint32_t> nextExplicitConsumerId{}
								,globalExplicitConsumerOffset{};

#ifdef MCDBGQ_NOLOCKFREE_IMPLICITPRODHASH
		debug::DebugMutex implicitProdMutex{};
#endif

#ifdef MOODYCAMEL_QUEUE_INTERNAL_DEBUG
		XAtomicPointer<ExplicitProducer> explicitProducers{};
		XAtomicPointer<ImplicitProducer> implicitProducers{};
#endif

		///////////////////////////////
		// Queue methods
		///////////////////////////////

		template<AllocationMode canAlloc, typename U>
		constexpr bool inner_enqueue(producer_token_t const & token, U && element)
		{ return static_cast<ExplicitProducer*>(token.producer)->XConcurrentQueueAbstract::ExplicitProducer::template enqueue<canAlloc>(std::forward<U>(element)); }

		template<AllocationMode canAlloc, typename U>
		constexpr bool inner_enqueue(U && element) {
			auto const producer { get_or_add_implicit_producer() };
			return producer
				? producer->XConcurrentQueueAbstract::ImplicitProducer::template enqueue<canAlloc>(std::forward<U>(element))
				: false;
		}

		template<AllocationMode canAlloc, typename It>
		static constexpr bool inner_enqueue_bulk(producer_token_t const & token, It && itemFirst, size_t const count)
		{ return static_cast<ExplicitProducer*>(token.producer)->XConcurrentQueueAbstract::ExplicitProducer::template enqueue_bulk<canAlloc>(std::forward<decltype(itemFirst)>(itemFirst), count); }

		template<AllocationMode canAlloc, typename It>
		constexpr bool inner_enqueue_bulk(It && itemFirst, size_t const count) {
			auto const producer { get_or_add_implicit_producer() };
			return producer
				? producer->ImplicitProducer::template enqueue_bulk<canAlloc>(std::forward<decltype(itemFirst)>(itemFirst), count)
				: false;
		}

		constexpr bool update_current_producer_after_rotation(consumer_token_t & token) {
			// Ah, there's been a rotation, figure out where we should be!
			auto const tail{ producerListTail.loadAcquire() };
			if (!token.desiredProducer && !tail ) { return {}; }
			auto const prodCount{ producerCount.loadRelaxed() };
			auto const globalOffset{ globalExplicitConsumerOffset.loadRelaxed() };
			if ((details::unlikely)(!token.desiredProducer)) {
				// Aha, first time we're dequeueing anything.
				// Figure out our local position
				// Note: offset is from start, not end, but we're traversing from end -- subtract from count first
				auto const offset{ prodCount - 1 - token.initialOffset % prodCount};
				token.desiredProducer = tail;
				for (std::uint32_t i {}; i != offset; ++i) {
					token.desiredProducer = static_cast<ProducerBase*>(token.desiredProducer)->next_prod();
					if (!token.desiredProducer) { token.desiredProducer = tail; }
				}
			}

			auto delta{ globalOffset - token.lastKnownGlobalOffset };
			if (delta >= prodCount) { delta = delta % prodCount; }
			for (std::uint32_t i {}; i != delta; ++i) {
				token.desiredProducer = static_cast<ProducerBase*>(token.desiredProducer)->next_prod();
				if (!token.desiredProducer) { token.desiredProducer = tail; }
			}

			token.lastKnownGlobalOffset = globalOffset;
			token.currentProducer = token.desiredProducer;
			token.itemsConsumedFromCurrent = {};
			return true;
		}

		//////////////////////////////////
		// Block pool manipulation
		//////////////////////////////////

		constexpr void populate_initial_block_list(size_t const blockCount) {
			initialBlockPoolSize = blockCount;
			if (!initialBlockPoolSize) { initialBlockPool = {}; return;}

			initialBlockPool = create_array<Block>(blockCount);
			if (!initialBlockPool) { initialBlockPoolSize = {}; }

			for (size_t i {}; i < initialBlockPoolSize; ++i)
			{ initialBlockPool[i].dynamicallyAllocated = {}; }
		}

		constexpr Block * try_get_block_from_initial_pool() {
			if (initialBlockPoolIndex.loadRelaxed() >= initialBlockPoolSize)
			{ return {}; }
			auto const index { initialBlockPoolIndex.fetchAndAddRelaxed(1) };
			return index < initialBlockPoolSize ? initialBlockPool + index : nullptr;
		}

		constexpr void add_block_to_free_list(Block * const block) {
#ifdef MCDBGQ_TRACKMEM
			block->owner = {};
#endif
			!Traits::RECYCLE_ALLOCATED_BLOCKS && block->dynamicallyAllocated
				? destroy(block)
				: freeList.add(block);
		}

		constexpr void add_blocks_to_free_list(Block * block) {
			while (block) {
				auto const next{ block->next };
				add_block_to_free_list(block);
				block = next;
			}
		}

		constexpr Block * try_get_block_from_free_list()
		{ return freeList.try_get(); }

		// Gets a free block from one of the memory pools, or allocates a new one (if applicable)
		template<AllocationMode canAlloc>
		Block * requisition_block() {
			auto block{ try_get_block_from_initial_pool() };
			if (block) { return block; }

			block = try_get_block_from_free_list();
			if (block) { return block; }

			MOODYCAMEL_CONSTEXPR_IF (canAlloc == CanAlloc)
			{ return create<Block>(); }
			else { return {}; }
		}

		//////////////////////////////////
		// Producer list manipulation
		//////////////////////////////////

		constexpr ProducerBase * recycle_or_create_producer(bool const isExplicit) {
#ifdef MCDBGQ_NOLOCKFREE_IMPLICITPRODHASH
			debug::DebugLock lock(implicitProdMutex);
#endif
			// Try to re-use one first
			for (auto ptr{ producerListTail.loadAcquire() }; ptr; ptr = ptr->next_prod()) {
				if (ptr->inactive.loadRelaxed() && ptr->isExplicit == isExplicit) {
					if (auto expected {true};
						ptr->inactive.m_x_value.compare_exchange_strong(expected, /* desired */ false
							, std::memory_order_acquire, std::memory_order_relaxed))
					{
						// We caught one! It's been marked as activated, the caller can have it
						return ptr;
					}
				}
			}

			return add_producer(isExplicit ? static_cast<ProducerBase*>(create<ExplicitProducer>(this)) : create<ImplicitProducer>(this));
		}

		constexpr ProducerBase* add_producer(ProducerBase * const producer) {
			// Handle failed memory allocation
			if (!producer ) { return {}; }

			producerCount.fetchAndAddRelaxed(1);

			// Add it to the lock-free list
			auto prevTail{ producerListTail.loadRelaxed() };
			do {
				producer->next = prevTail;
			} while (!producerListTail.m_x_value.compare_exchange_weak(prevTail, producer, std::memory_order_release, std::memory_order_relaxed));

#ifdef MOODYCAMEL_QUEUE_INTERNAL_DEBUG
			if (producer->isExplicit) {
				auto prevTailExplicit{ explicitProducers.loadRelaxed() };
				do {
					static_cast<ExplicitProducer*>(producer)->nextExplicitProducer = prevTailExplicit;
				} while (!explicitProducers.m_x_value.compare_exchange_weak(prevTailExplicit, static_cast<ExplicitProducer*>(producer), std::memory_order_release, std::memory_order_relaxed));
			}else {
				auto prevTailImplicit{ implicitProducers.loadRelaxed() };
				do {
					static_cast<ImplicitProducer*>(producer)->nextImplicitProducer = prevTailImplicit;
				} while (!implicitProducers.m_x_value.compare_exchange_weak(prevTailImplicit, static_cast<ImplicitProducer*>(producer), std::memory_order_release, std::memory_order_relaxed));
			}
#endif

			return producer;
		}

		constexpr void reownProducers() noexcept{
			// After another instance is moved-into/swapped-with this one, all the
			// producers we stole still think their parents are the other queue.
			// So fix them up!
			for (auto ptr{ producerListTail.loadRelaxed() }; ptr; ptr = ptr->next_prod())
			{ ptr->parent = this; }
		}

		constexpr void populate_initial_implicit_producer_hash() {
			MOODYCAMEL_CONSTEXPR_IF (INITIAL_IMPLICIT_PRODUCER_HASH_SIZE) {
				implicitProducerHashCount.storeRelaxed({});
				auto const hash { std::addressof(initialImplicitProducerHash) };
				hash->capacity = INITIAL_IMPLICIT_PRODUCER_HASH_SIZE;
				hash->entries = initialImplicitProducerHashEntries.data();
				for (size_t i {}; i != INITIAL_IMPLICIT_PRODUCER_HASH_SIZE; ++i)
				{ initialImplicitProducerHashEntries[i].key.store(details::invalid_thread_id, std::memory_order_relaxed); }
				hash->prev = {};
				implicitProducerHash.storeRelaxed(hash);
			}
		}

		constexpr void swap_implicit_producer_hashes(XConcurrentQueueAbstract & other) {

			if constexpr (INITIAL_IMPLICIT_PRODUCER_HASH_SIZE) {
				// Swap (assumes our implicit producer hash is initialized)
				initialImplicitProducerHashEntries.swap(other.initialImplicitProducerHashEntries);
				initialImplicitProducerHash.entries = initialImplicitProducerHashEntries.data();
				other.initialImplicitProducerHash.entries = other.initialImplicitProducerHashEntries.data();

				details::swap_relaxed(implicitProducerHashCount.m_x_value, other.implicitProducerHashCount.m_x_value);
				details::swap_relaxed(implicitProducerHash.m_x_value, other.implicitProducerHash.m_x_value);

				auto const ImplicitProducerHashPtr { std::addressof(initialImplicitProducerHash) };
				auto const otherImplicitProducerHashPtr { std::addressof(other.initialImplicitProducerHash) };

				if (implicitProducerHash.loadRelaxed() == otherImplicitProducerHashPtr) {
					implicitProducerHash.storeRelaxed(ImplicitProducerHashPtr);
				}else {
					auto hash { implicitProducerHash.loadRelaxed() };
					for (;hash->prev != otherImplicitProducerHashPtr; hash = hash->prev) {}
					hash->prev = ImplicitProducerHashPtr;
				}

				if (other.implicitProducerHash.loadRelaxed() == ImplicitProducerHashPtr) {
					other.implicitProducerHash.storeRelaxed(otherImplicitProducerHashPtr);
				}else {
					auto hash { other.implicitProducerHash.loadRelaxed() };
					for (;hash->prev != ImplicitProducerHashPtr; hash = hash->prev) {}
					hash->prev = otherImplicitProducerHashPtr;
				}
			}
		}

		// Only fails (returns nullptr) if memory allocation fails
		constexpr ImplicitProducer * get_or_add_implicit_producer() {
			// Note that since the data is essentially thread-local (key is thread ID),
			// there's a reduced need for fences (memory ordering is already consistent
			// for any individual thread), except for the current table itself.

			// Start by looking for the thread ID in the current and all previous hash tables.
			// If it's not found, it must not be in there yet, since this same thread would
			// have added it previously to one of the tables that we traversed.

			// Code and algorithm adapted from http://preshing.com/20130605/the-worlds-simplest-lock-free-hash-table

#ifdef MCDBGQ_NOLOCKFREE_IMPLICITPRODHASH
			debug::DebugLock lock { implicitProdMutex };
#endif

			auto const id{ details::thread_id() };
			auto const hashedId{ details::hash_thread_id(id) };

			auto mainHash{ implicitProducerHash.loadAcquire() };
			assert(mainHash);  // silence clang-tidy and MSVC warnings (hash cannot be null)
			for (auto hash { mainHash }; hash; hash = hash->prev) {
				// Look for the id in this hash
				auto index{ hashedId };
				while (true) {		// Not an infinite loop because at least one slot is free in the hash table
					index &= hash->capacity - 1u;

					auto const probedKey{ hash->entries[index].key.load(std::memory_order_relaxed) };
					if (probedKey == id) {
						// Found it! If we had to search several hashes deep, though, we should lazily add it
						// to the current main hash table to avoid the extended search next time.
						// Note there's guaranteed to be room in the current hash table since every subsequent
						// table implicitly reserves space for all previous tables (there's only one
						// implicitProducerHashCount).
						auto const value{ hash->entries[index].value };
						if (hash != mainHash) {
							index = hashedId;
							while (true) {
								index &= mainHash->capacity - 1u;
								auto empty{ details::invalid_thread_id};
#ifdef MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED
								auto reusable { details::invalid_thread_id2 };
								if (mainHash->entries[index].key.compare_exchange_strong(empty,    id, std::memory_order_seq_cst, std::memory_order_relaxed) ||
									mainHash->entries[index].key.compare_exchange_strong(reusable, id, std::memory_order_seq_cst, std::memory_order_relaxed)) {
#else
								if (mainHash->entries[index].key.compare_exchange_strong(empty,id, std::memory_order_seq_cst, std::memory_order_relaxed)) {
#endif
									mainHash->entries[index].value = value;
									break;
								}
								++index;
							}
						}

						return value;
					}
					if (details::invalid_thread_id == probedKey) { break; } // Not in this hash table
					++index;
				}
			}

			// Insert!
			auto const newCount{ 1 + implicitProducerHashCount.fetchAndAddRelaxed(1) };
			while (true) {
				// NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
				if (newCount >= mainHash->capacity >> 1 && !implicitProducerHashResizeInProgress.test_and_set(std::memory_order_acquire)) {
					// We've acquired the resize lock, try to allocate a bigger hash table.
					// Note the acquire fence synchronizes with the release fence at the end of this block, and hence when
					// we reload implicitProducerHash it must be the most recent version (it only gets changed within this
					// locked block).
					mainHash = implicitProducerHash.loadAcquire();
					if (newCount >= mainHash->capacity >> 1) {
						auto newCapacity{ mainHash->capacity << 1 };
						while (newCount >= newCapacity >> 1) { newCapacity <<= 1; }
						auto const raw {
							static_cast<char*>((Traits::malloc)(sizeof(ImplicitProducerHash)
								+ std::alignment_of_v<ImplicitProducerKVP> - 1
								+ sizeof(ImplicitProducerKVP) * newCapacity))
						};
						if (!raw) {
							// Allocation failed
							implicitProducerHashCount.fetchAndSubRelaxed(1);
							implicitProducerHashResizeInProgress.clear(std::memory_order_relaxed);
							return {};
						}

						auto newHash { new (raw) ImplicitProducerHash };
						newHash->capacity = static_cast<size_t>(newCapacity);
						newHash->entries = reinterpret_cast<ImplicitProducerKVP*>(details::align_for<ImplicitProducerKVP>(raw + sizeof(ImplicitProducerHash)));
						for (size_t i {}; i != newCapacity; ++i) {
							new (newHash->entries + i) ImplicitProducerKVP;
							newHash->entries[i].key.store(details::invalid_thread_id, std::memory_order_relaxed);
						}
						newHash->prev = mainHash;
						implicitProducerHash.storeRelease(newHash);
						implicitProducerHashResizeInProgress.clear(std::memory_order_release);
						mainHash = newHash;
					}else {
						implicitProducerHashResizeInProgress.clear(std::memory_order_release);
					}
				}

				// If it's < three-quarters full, add to the old one anyway so that we don't have to wait for the next table
				// to finish being allocated by another thread (and if we just finished allocating above, the condition will
				// always be true)
				if (newCount < (mainHash->capacity >> 1) + (mainHash->capacity >> 2)) {

					auto const producer{ static_cast<ImplicitProducer*>(recycle_or_create_producer({}))};

					if (!producer) {
						implicitProducerHashCount.fetchAndSubRelaxed(1);
						return {};
					}

#ifdef MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED
					producer->threadExitListener.callback = std::addressof(implicit_producer_thread_exited_callback);
					producer->threadExitListener.userData = producer;
					details::ThreadExitNotifier::subscribe(std::addressof(producer->threadExitListener));
#endif

					auto index{ hashedId };
					while (true) {
						index &= mainHash->capacity - 1u;
						auto empty{ details::invalid_thread_id };
#ifdef MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED
						auto reusable { details::invalid_thread_id2 };
						if (mainHash->entries[index].key.compare_exchange_strong(reusable, id, std::memory_order_seq_cst, std::memory_order_relaxed)) {
							implicitProducerHashCount.fetchAndSubRelaxed(1);  // already counted as a used slot
							mainHash->entries[index].value = producer;
							break;
						}
#endif
						if (mainHash->entries[index].key.compare_exchange_strong(empty,id, std::memory_order_seq_cst, std::memory_order_relaxed)) {
							mainHash->entries[index].value = producer;
							break;
						}
						++index;
					}
					return producer;
				}

				// Hmm, the old hash is quite full and somebody else is busy allocating a new one.
				// We need to wait for the allocating thread to finish (if it succeeds, we add, if not,
				// we try to allocate ourselves).
				mainHash = implicitProducerHash.loadAcquire();
			}
		}

#ifdef MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED
		constexpr void implicit_producer_thread_exited(ImplicitProducer * const producer) {
			// Remove from hash
#ifdef MCDBGQ_NOLOCKFREE_IMPLICITPRODHASH
			debug::DebugLock lock { implicitProdMutex };
#endif
			auto hash { implicitProducerHash.loadAcquire() };
			assert(hash);		// The thread exit listener is only registered if we were added to a hash in the first place
			auto const id { details::thread_id() };
			auto const hashedId { details::hash_thread_id(id) };
			details::thread_id_t probedKey;

			// We need to traverse all the hashes just in case other threads aren't on the current one yet and are
			// trying to add an entry thinking there's a free slot (because they reused a producer)
			for (; hash; hash = hash->prev) {
				auto index { hashedId };
				do {
					index &= hash->capacity - 1u;
					probedKey = id;
					if (hash->entries[index].key.compare_exchange_strong(probedKey, details::invalid_thread_id2, std::memory_order_seq_cst, std::memory_order_relaxed))
					{ break; }
					++index;
				} while (probedKey != details::invalid_thread_id);		// Can happen if the hash has changed but we weren't put back in it yet, or if we weren't added to this hash in the first place
			}

			// Mark the queue as being recyclable
			producer->inactive.storeRelease(true);
		}

		static constexpr void implicit_producer_thread_exited_callback(void * const userData) {
			auto const producer{ static_cast<ImplicitProducer*>(userData) };
			auto const queue { producer->parent };
			queue->implicit_producer_thread_exited(producer);
		}
#endif

		XConcurrentQueueAbstract & swap_internal(XConcurrentQueueAbstract & other) {
			if (this == std::addressof(other)) { return *this; }
			details::swap_relaxed(producerListTail.m_x_value, other.producerListTail.m_x_value);
			details::swap_relaxed(producerCount.m_x_value , other.producerCount.m_x_value);
			details::swap_relaxed(initialBlockPoolIndex.m_x_value, other.initialBlockPoolIndex.m_x_value);
			std::swap(initialBlockPool, other.initialBlockPool);
			std::swap(initialBlockPoolSize, other.initialBlockPoolSize);
			freeList.swap(other.freeList);
			details::swap_relaxed(nextExplicitConsumerId.m_x_value, other.nextExplicitConsumerId.m_x_value);
			details::swap_relaxed(globalExplicitConsumerOffset.m_x_value, other.globalExplicitConsumerOffset.m_x_value);

			swap_implicit_producer_hashes(other);

			reownProducers();
			other.reownProducers();

#ifdef MOODYCAMEL_QUEUE_INTERNAL_DEBUG
			details::swap_relaxed(explicitProducers.m_x_value, other.explicitProducers.m_x_value);
			details::swap_relaxed(implicitProducers.m_x_value, other.implicitProducers.m_x_value);
#endif
			return *this;
		}

		//////////////////////////////////
		// Utility functions
		//////////////////////////////////

		template<typename TAlign>
		static constexpr void * aligned_malloc(size_t const size) {
			MOODYCAMEL_CONSTEXPR_IF (std::alignment_of_v<TAlign> <= std::alignment_of_v<details::max_align_t>)
			{ return (Traits::malloc)(size); }
			else {
				auto const raw{ (Traits::malloc)(size + std::alignment_of_v<TAlign> - 1 + sizeof(void*)) };
				if (!raw) {return {};}
				auto const ptr{ details::align_for<TAlign>(reinterpret_cast<char*>(raw) + sizeof(void*)) };
				*(reinterpret_cast<void**>(ptr) - 1) = raw;
				return ptr;
			}
		}

		template<typename TAlign>
		static constexpr void aligned_free(void * const ptr) noexcept {
			if constexpr (std::alignment_of_v<TAlign> <= std::alignment_of_v<details::max_align_t>)
			{ (Traits::free)(ptr); }
			else {(Traits::free)(ptr ? *(static_cast<void**>(ptr) - 1) : nullptr); }
		}

		template<typename U>
		static constexpr U * create_array(size_t const count) {
			assert(count > 0);
			auto const p { static_cast<U*>(aligned_malloc<U>(sizeof(U) * count)) };
			if (!p) {return {};}
			for (size_t i {}; i != count; ++i){ new (p + i) U(); }
			return p;
		}

		template<typename U>
		static constexpr void destroy_array(U * const p, size_t const count) {
			if (p) { assert(count > 0); for(size_t i {count}; i != 0; ) { (p + --i)->~U(); } }
			aligned_free<U>(p);
		}

		template<typename U>
		static constexpr U * create() {
			auto const p{ aligned_malloc<U>(sizeof(U)) };
			return p ? new (p) U : nullptr;
		}

		template<typename U, typename A1>
		static U * create(A1 && a1) {
			auto const p{ aligned_malloc<U>(sizeof(U)) };
			return p ? new (p) U(std::forward<A1>(a1)) : nullptr;
		}

		template<typename U>
		static constexpr void destroy(U * const p)
		{ if (p) { p->~U(); } aligned_free<U>(p); }

	public:
		X_DISABLE_COPY(XConcurrentQueueAbstract)

		virtual ~XConcurrentQueueAbstract() {
			// Destroy producers
			auto ptr{ producerListTail.loadRelaxed() };
			while (ptr) {
				auto next{ ptr->next_prod() };
				if (ptr->token ) { ptr->token->producer = {}; }
				destroy(ptr);
				ptr = next;
			}

			// Destroy implicit producer hash tables
			MOODYCAMEL_CONSTEXPR_IF (INITIAL_IMPLICIT_PRODUCER_HASH_SIZE) {
				auto hash { implicitProducerHash.loadRelaxed() };
				while (hash) {
					auto prev{hash->prev};
					if (prev) {		// The last hash is part of this object and was not allocated dynamically
						for (size_t i {}; i != hash->capacity; ++i)
						{ hash->entries[i].~ImplicitProducerKVP(); }
						hash->~ImplicitProducerHash();
						(Traits::free)(hash);
					}
					hash = prev;
				}
			}

			// Destroy global free list
			auto block { freeList.head_unsafe() };
			while (block) {
				auto next{ block->freeListNext.loadRelaxed() };
				if (block->dynamicallyAllocated) { destroy(block); }
				block = next;
			}

			// Destroy initial free list
			destroy_array(initialBlockPool,initialBlockPoolSize);
		}

		XConcurrentQueueAbstract (XConcurrentQueueAbstract && other) noexcept :
			producerListTail(other.producerListTail.loadRelaxed()),
			producerCount(other.producerCount.loadRelaxed()),
			initialBlockPoolIndex(other.initialBlockPoolIndex.loadRelaxed()),
			initialBlockPool(other.initialBlockPool),
			initialBlockPoolSize(other.initialBlockPoolSize),
			freeList(std::move(other.freeList)),
			nextExplicitConsumerId(other.nextExplicitConsumerId.loadRelaxed()),
			globalExplicitConsumerOffset(other.globalExplicitConsumerOffset.loadRelaxed())
		{
			// Move the other one into this, and leave the other one as an empty queue
			implicitProducerHashResizeInProgress.clear(std::memory_order_relaxed);
			populate_initial_implicit_producer_hash();
			swap_implicit_producer_hashes(other);

			other.producerListTail.storeRelaxed({});
			other.producerCount.storeRelaxed({});
			other.nextExplicitConsumerId.storeRelaxed({});
			other.globalExplicitConsumerOffset.storeRelaxed({});

#ifdef MOODYCAMEL_QUEUE_INTERNAL_DEBUG
			explicitProducers.storeRelaxed(other.explicitProducers.loadRelaxed());
			other.explicitProducers.storeRelaxed({});

			implicitProducers.storeRelaxed(other.implicitProducers.loadRelaxed());
			other.implicitProducers.storeRelaxed({});
#endif

			other.initialBlockPoolIndex.storeRelaxed({});
			other.initialBlockPoolSize = {};
			other.initialBlockPool = {};

			reownProducers();
		}

		XConcurrentQueueAbstract & operator=(XConcurrentQueueAbstract && other) MOODYCAMEL_NOEXCEPT
		{ swap_internal(other); return *this;}

		// Swaps this queue's state with the other's. Not thread-safe.
		// Swapping two queues does not invalidate their tokens, however
		// the tokens that were created for one queue must be used with
		// only the swapped queue (i.e. the tokens are tied to the
		// queue's movable state, not the object itself).
		constexpr void swap(XConcurrentQueueAbstract & other) MOODYCAMEL_NOEXCEPT
		{ this->swap_internal(other); }

	private:
		constexpr XConcurrentQueueAbstract() = default;
		friend struct ProducerToken;
		friend struct ConsumerToken;
		friend struct ExplicitProducer;
		friend struct ImplicitProducer;
		friend class XConcurrentQueueTests;
	};
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
