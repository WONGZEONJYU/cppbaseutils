#ifndef XUTILS2_MOODYCAMEL_MACROS_HPP
#define XUTILS2_MOODYCAMEL_MACROS_HPP 1

#pragma once

#ifndef XUTILS2_MOODYCAMEL_MACROS_HPP_
	#error "moodycamelmacros.hpp is internal header file!"
#endif

#ifdef MCDBGQ_USE_RELACY
#include "relacy/relacy_std.hpp"
#include "relacy_shims.h"
// We only use malloc/free anyway, and the delete macro messes up `= delete` method declarations.
// We'll override the default trait malloc ourselves without a macro.
#undef new
#undef delete
#undef malloc
#undef free
#endif

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif
#include <thread>		// partly for __WINPTHREADS_VERSION if on MinGW-w64 w/ POSIX threading
#include <XHelper/xversion.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

// Platform-specific definitions of a numeric thread ID type and an invalid value
namespace moodycamel::details {
    template<typename thread_id_t> struct thread_id_converter {
        using thread_id_numeric_size_t = thread_id_t ;
        using thread_id_hash_t = thread_id_t ;
        static constexpr thread_id_hash_t preHash(thread_id_t const & x) noexcept { return x; }
    };
}

#if defined(MCDBGQ_USE_RELACY)

	namespace moodycamel::details {
		using thread_id_t = std::uint32_t ;
		inline constexpr thread_id_t invalid_thread_id { 0xFFFFFFFFU },
						 thread_id_t invalid_thread_id2 { 0xFFFFFFFEU };
		static constexpr thread_id_t thread_id() noexcept { return rl::thread_index(); }
	}

#elif defined(_WIN32) || defined(__WINDOWS__) || defined(__WIN32__)
// No sense pulling in windows.h in a header, we'll manually declare the function
// we use and rely on backwards-compatibility for this not to break

	extern "C" {
		__declspec(dllimport) unsigned long __stdcall GetCurrentThreadId(void);
	}

	namespace moodycamel::details {
		static_assert(sizeof(unsigned long) == sizeof(std::uint32_t), "Expected size of unsigned long to be 32 bits on Windows");
		using thread_id_t = std::uint32_t ;
		inline constexpr thread_id_t invalid_thread_id { 0u },			// See http://blogs.msdn.com/b/oldnewthing/archive/2004/02/23/78395.aspx
									invalid_thread_id2 { 0xFFFFFFFFU };	// Not technically guaranteed to be invalid, but is never used in practice. Note that all Win32 thread IDs are presently multiples of 4.
		static thread_id_t thread_id() noexcept { return static_cast<thread_id_t>( GetCurrentThreadId()); }
	}

#elif defined(__arm__) || defined(_M_ARM) || defined(__aarch64__) || (defined(__APPLE__) && TARGET_OS_IPHONE) || defined(__MVS__) || defined(MOODYCAMEL_NO_THREAD_LOCAL)

	namespace moodycamel::details {

		static_assert(sizeof(std::thread::id) == 4 || sizeof(std::thread::id) == 8, "std::thread::id is expected to be either 4 or 8 bytes");

		using thread_id_t = std::thread::id;
		inline const thread_id_t invalid_thread_id {};         // Default ctor creates invalid ID

		// Note we don't define a invalid_thread_id2 since std::thread::id doesn't have one; it's
		// only used if MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED is defined anyway, which it won't
		// be.
		[[maybe_unused]] static constexpr thread_id_t thread_id() noexcept { return std::this_thread::get_id(); }

		template<std::size_t> struct thread_id_size { };
		template<> struct thread_id_size<4> { using numeric_t = std::uint32_t; };
		template<> struct thread_id_size<8> { using numeric_t = std::uint64_t; };

		template<> struct thread_id_converter<thread_id_t> {
			using thread_id_numeric_size_t = thread_id_size<sizeof(thread_id_t)>::numeric_t;
	#ifndef __APPLE__
			using thread_id_hash_t = std::size_t;
	#else
			using thread_id_hash_t = thread_id_numeric_size_t;
	#endif

			static constexpr thread_id_hash_t preHash(thread_id_t const & x) noexcept {
	#ifndef __APPLE__
				return std::hash<std::thread::id>()(x);
	#else
				return *reinterpret_cast<thread_id_hash_t const*>(&x);
	#endif
			}
		};
	}

#else
// Use a nice trick from this answer: http://stackoverflow.com/a/8438730/21475
// In order to get a numeric thread ID in a platform-independent way, we use a thread-local
// static variable's address as a thread identifier :-)

	#if defined(__GNUC__) || defined(__INTEL_COMPILER)
		#define MOODYCAMEL_THREADLOCAL __thread
	#elif defined(_MSC_VER)
		#define MOODYCAMEL_THREADLOCAL __declspec(thread)
	#else
	// Assume C++11 compliant compiler
		#define MOODYCAMEL_THREADLOCAL thread_local
	#endif
	namespace moodycamel::details {
		using thread_id_t = std::uintptr_t;
		inline constexpr thread_id_t invalid_thread_id {0},		// Address can't be nullptr
						thread_id_t invalid_thread_id2 {1};		// Member accesses off a null pointer are also generally invalid. Plus it's not aligned.
		static constexpr thread_id_t thread_id() noexcept { static MOODYCAMEL_THREADLOCAL int x; return reinterpret_cast<thread_id_t>(&x); }
	}

#endif

// Constexpr if
#ifndef MOODYCAMEL_CONSTEXPR_IF
	#if (defined(_MSC_VER) && defined(_HAS_CXX17) && _HAS_CXX17) || __cplusplus > 201402L
		#define MOODYCAMEL_CONSTEXPR_IF if constexpr
		#define MOODYCAMEL_MAYBE_UNUSED [[maybe_unused]]
	#else
		#define MOODYCAMEL_CONSTEXPR_IF if
		#define MOODYCAMEL_MAYBE_UNUSED
	#endif
#endif

// Exceptions
#ifndef MOODYCAMEL_EXCEPTIONS_ENABLED
	#if (defined(_MSC_VER) && defined(_CPPUNWIND)) || (defined(__GNUC__) && defined(__EXCEPTIONS)) || (!defined(_MSC_VER) && !defined(__GNUC__))
		#define MOODYCAMEL_EXCEPTIONS_ENABLED
	#endif
#endif

#ifdef MOODYCAMEL_EXCEPTIONS_ENABLED
	#define MOODYCAMEL_TRY try
	#define MOODYCAMEL_CATCH(...) catch(__VA_ARGS__)
	#define MOODYCAMEL_RETHROW throw
	#define MOODYCAMEL_THROW(expr) throw (expr)
#else
	#define MOODYCAMEL_TRY MOODYCAMEL_CONSTEXPR_IF (true)
	#define MOODYCAMEL_CATCH(...) else MOODYCAMEL_CONSTEXPR_IF (false)
	#define MOODYCAMEL_RETHROW
	#define MOODYCAMEL_THROW(expr)
#endif

#ifndef MOODYCAMEL_NOEXCEPT
	#if !defined(MOODYCAMEL_EXCEPTIONS_ENABLED)
		#define MOODYCAMEL_NOEXCEPT
		#define MOODYCAMEL_NOEXCEPT_CTOR(type, valueType, expr) true
		#define MOODYCAMEL_NOEXCEPT_ASSIGN(type, valueType, expr) true
	#elif defined(_MSC_VER) && defined(_NOEXCEPT) && _MSC_VER < 1800
// VS2012's std::is_nothrow_[move_]constructible is broken and returns true when it shouldn't :-(
// We have to assume *all* non-trivial constructors may throw on VS2012!
		#define MOODYCAMEL_NOEXCEPT _NOEXCEPT
		#define MOODYCAMEL_NOEXCEPT_CTOR(type, valueType, expr) (std::is_rvalue_reference_v<valueType> && std::is_move_constructible_v<type> ? std::is_trivially_move_constructible_v<type> : std::is_trivially_copy_constructible<type>_v)
		#define MOODYCAMEL_NOEXCEPT_ASSIGN(type, valueType, expr) ((std::is_rvalue_reference_v<valueType> && std::is_move_assignable_v<type> ? std::is_trivially_move_assignable_v<type> || std::is_nothrow_move_assignable_v<type> : std::is_trivially_copy_assignable_v<type> || std::is_nothrow_copy_assignable_v<type>) && MOODYCAMEL_NOEXCEPT_CTOR(type, valueType, expr))
	#elif defined(_MSC_VER) && defined(_NOEXCEPT) && _MSC_VER < 1900
		#define MOODYCAMEL_NOEXCEPT _NOEXCEPT
		#define MOODYCAMEL_NOEXCEPT_CTOR(type, valueType, expr) (std::is_rvalue_reference_v<valueType> && std::is_move_constructible_v<type> ? std::is_trivially_move_constructible_v<type> || std::is_nothrow_move_constructible_v<type> : std::is_trivially_copy_constructible_v<type> || std::is_nothrow_copy_constructible_v<type>)
		#define MOODYCAMEL_NOEXCEPT_ASSIGN(type, valueType, expr) ((std::is_rvalue_reference_v<valueType> && std::is_move_assignable_v<type> ? std::is_trivially_move_assignable_v<type> || std::is_nothrow_move_assignable_v<type> : std::is_trivially_copy_assignable_v<type> || std::is_nothrow_copy_assignable_v<type>) && MOODYCAMEL_NOEXCEPT_CTOR(type, valueType, expr))
	#else
		#define MOODYCAMEL_NOEXCEPT noexcept
		#define MOODYCAMEL_NOEXCEPT_CTOR(type, valueType, expr) noexcept(expr)
		#define MOODYCAMEL_NOEXCEPT_ASSIGN(type, valueType, expr) noexcept(expr)
	#endif
#endif

#ifndef MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED
	#ifdef MCDBGQ_USE_RELACY
		#define MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED
	#else
	// VS2013 doesn't support `thread_local`, and MinGW-w64 w/ POSIX threading has a crippling bug: http://sourceforge.net/p/mingw-w64/bugs/445
	// g++ <=4.7 doesn't support thread_local either.
	// Finally, iOS/ARM doesn't have support for it either, and g++/ARM allows it to compile but it's unconfirmed to actually work
		#if (!defined(_MSC_VER) || _MSC_VER >= 1900) && (!defined(__MINGW32__) && !defined(__MINGW64__) || !defined(__WINPTHREADS_VERSION)) && (!defined(__GNUC__) || __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)) && (!defined(__APPLE__) || !TARGET_OS_IPHONE) && !defined(__arm__) && !defined(_M_ARM) && !defined(__aarch64__) && !defined(__MVS__)
		// Assume `thread_local` is fully supported in all other C++11 compilers/platforms
			#define MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED    // tentatively enabled for now; years ago several users report having problems with it on
		#endif
	#endif
#endif

// VS2012 doesn't support deleted functions.
// In this case, we declare the function normally but don't define it. A link error will be generated if the function is called.
#ifndef MOODYCAMEL_DELETE_FUNCTION
	#if defined(_MSC_VER) && _MSC_VER < 1800
		#define MOODYCAMEL_DELETE_FUNCTION
	#else
		#define MOODYCAMEL_DELETE_FUNCTION = delete
	#endif
#endif

namespace moodycamel::details {
#ifndef MOODYCAMEL_ALIGNAS
	// VS2013 doesn't support alignas or alignof, and align() requires a constant literal
	#if defined(_MSC_VER) && _MSC_VER <= 1800
		#define MOODYCAMEL_ALIGNAS(alignment) __declspec(align(alignment))
		#define MOODYCAMEL_ALIGNOF(obj) __alignof(obj)
		#define MOODYCAMEL_ALIGNED_TYPE_LIKE(T, obj) typename details::Vs2013Aligned<std::alignment_of<obj>::value, T>::type

		template<int Align, typename T> struct Vs2013Aligned { };  // default, unsupported alignment
	#if 0
		template<typename T> struct Vs2013Aligned<1, T> { typedef __declspec(align(1)) T type; };
		template<typename T> struct Vs2013Aligned<2, T> { typedef __declspec(align(2)) T type; };
		template<typename T> struct Vs2013Aligned<4, T> { typedef __declspec(align(4)) T type; };
		template<typename T> struct Vs2013Aligned<8, T> { typedef __declspec(align(8)) T type; };
		template<typename T> struct Vs2013Aligned<16, T> { typedef __declspec(align(16)) T type; };
		template<typename T> struct Vs2013Aligned<32, T> { typedef __declspec(align(32)) T type; };
		template<typename T> struct Vs2013Aligned<64, T> { typedef __declspec(align(64)) T type; };
		template<typename T> struct Vs2013Aligned<128, T> { typedef __declspec(align(128)) T type; };
		template<typename T> struct Vs2013Aligned<256, T> { typedef __declspec(align(256)) T type; };
	#else
		template<typename T> struct Vs2013Aligned<1, T> { using type = __declspec(align(1)) T; };
		template<typename T> struct Vs2013Aligned<2, T> { using type = __declspec(align(2)) T ; };
		template<typename T> struct Vs2013Aligned<4, T> { using type = __declspec(align(4)) T ; };
		template<typename T> struct Vs2013Aligned<8, T> { using type = __declspec(align(8)) T ; };
		template<typename T> struct Vs2013Aligned<16, T> { using type = __declspec(align(16)) T; };
		template<typename T> struct Vs2013Aligned<32, T> { using type = __declspec(align(32)) T; };
		template<typename T> struct Vs2013Aligned<64, T> { using type = __declspec(align(64)) T; };
		template<typename T> struct Vs2013Aligned<128, T> { using type = __declspec(align(128)) T; };
		template<typename T> struct Vs2013Aligned<256, T> { using type = __declspec(align(256)) T; };
	#endif

	#else
		template<typename T> struct identity { using type = T; };
		template<typename T> using identity_t = identity<T>::type;
		#define MOODYCAMEL_ALIGNAS(alignment) alignas(alignment)
		#define MOODYCAMEL_ALIGNOF(obj) alignof(obj)
		#define MOODYCAMEL_ALIGNED_TYPE_LIKE(T, obj) alignas(alignof(obj))  moodycamel::details::identity_t<T>
	#endif
#endif
}

// TSAN can false report races in lock-free code.  To enable TSAN to be used from projects that use this one,
// we can apply per-function compile-time suppression.
// See https://clang.llvm.org/docs/ThreadSanitizer.html#has-feature-thread-sanitizer
#define MOODYCAMEL_NO_TSAN

#if defined(__has_feature)
	#if __has_feature(thread_sanitizer)
	#undef MOODYCAMEL_NO_TSAN
	#define MOODYCAMEL_NO_TSAN __attribute__((no_sanitize("thread")))
	#endif // TSAN
#endif // TSAN

// Compiler-specific likely/unlikely hints
namespace moodycamel::details {
#if defined(__GNUC__)
	[[maybe_unused]] static bool (likely)(bool const x) { return __builtin_expect(x, true); }
	[[maybe_unused]] static bool (unlikely)(bool const x) { return __builtin_expect(x, false); }
#else
	[[maybe_unused]] static bool (likely)(bool const x) noexcept { return x; }
	[[maybe_unused]] static bool (unlikely)(bool const x) noexcept { return x; }
#endif
}

namespace moodycamel::details {
		template<typename T>
		struct const_numeric_max {
			static_assert(std::is_integral_v<T>, "const_numeric_max can only be used with integers");
			static constexpr T value { std::numeric_limits<T>::is_signed
				? (static_cast<T>(1) << (sizeof(T) * CHAR_BIT - 1)) - static_cast<T>(1)
				: static_cast<T>(-1) };
		};

		template<typename T> using const_numeric_max_t = const_numeric_max<T>::type;
		template<typename T> inline constexpr auto const_numeric_max_v { const_numeric_max<T>::value };

#if defined(__GLIBCXX__)
		using std_max_align_t = ::max_align_t ;      // libstdc++ forgot to add it to std:: for a while
#else
		using std_max_align_t = std::max_align_t ;   // Others (e.g. MSVC) insist it can *only* be accessed via std::
#endif

		// Some platforms have incorrectly set max_align_t to a type with <8 bytes alignment even while supporting
		// 8-byte aligned scalar values (*cough* 32-bit iOS). Work around this with our own union. See issue #64.
		union max_align_t {
			std_max_align_t x {};
			long long y;
			void * z;
		};
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
