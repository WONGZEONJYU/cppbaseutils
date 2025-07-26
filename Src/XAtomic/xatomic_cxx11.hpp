#ifndef X_ATOMIC_CXX11_H
#define X_ATOMIC_CXX11_H 1

#include <atomic>
#include "xgenericatomic.hpp"

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

#if 0
/* Attempt to detect whether the atomic operations exist in hardware
 * or whether they are emulated by way of a lock.
 *
 * C++11 29.4 [atomics.lockfree] p1 says
 *
 *  The ATOMIC_..._LOCK_FREE macros indicate the lock-free property of the
 *  corresponding atomic types, with the signed and unsigned variants grouped
 *  together. The properties also apply to the corresponding (partial)
 *  specializations of the atomic template. A value of 0 indicates that the
 *  types are never lock-free. A value of 1 indicates that the types are
 *  sometimes lock-free. A value of 2 indicates that the types are always
 *  lock-free.
 *
 * We have a problem when the value is 1: we'd need to check at runtime, but
 * XAtomicInteger requires a constexpr answer (defect introduced in Qt 5.0). So
 * we'll err in the side of caution and say it isn't.
 */
#endif

template <int> struct XAtomicTraits
{ static inline bool isLockFree(); };

#define X_ATOMIC_INT32_IS_SUPPORTED
#if ATOMIC_INT_LOCK_FREE == 2
#  define X_ATOMIC_INT_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
#  define X_ATOMIC_INT_TEST_AND_SET_IS_ALWAYS_NATIVE
#  define X_ATOMIC_INT_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#  define X_ATOMIC_INT_FETCH_AND_ADD_IS_ALWAYS_NATIVE
#  define X_ATOMIC_INT32_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
#  define X_ATOMIC_INT32_TEST_AND_SET_IS_ALWAYS_NATIVE
#  define X_ATOMIC_INT32_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#  define X_ATOMIC_INT32_FETCH_AND_ADD_IS_ALWAYS_NATIVE

template <> inline bool XAtomicTraits<4>::isLockFree()
{ return true; }
#elif ATOMIC_INT_LOCK_FREE == 1
#  define X_ATOMIC_INT_REFERENCE_COUNTING_IS_SOMETIMES_NATIVE
#  define X_ATOMIC_INT_TEST_AND_SET_IS_SOMETIMES_NATIVE
#  define X_ATOMIC_INT_FETCH_AND_STORE_IS_SOMETIMES_NATIVE
#  define X_ATOMIC_INT_FETCH_AND_ADD_IS_SOMETIMES_NATIVE
#  define X_ATOMIC_INT32_REFERENCE_COUNTING_IS_SOMETIMES_NATIVE
#  define X_ATOMIC_INT32_TEST_AND_SET_IS_SOMETIMES_NATIVE
#  define X_ATOMIC_INT32_FETCH_AND_STORE_IS_SOMETIMES_NATIVE
#  define X_ATOMIC_INT32_FETCH_AND_ADD_IS_SOMETIMES_NATIVE

template <> inline bool XAtomicTraits<4>::isLockFree()
{ return false; }
#else
#  define X_ATOMIC_INT_REFERENCE_COUNTING_IS_NEVER_NATIVE
#  define X_ATOMIC_INT_TEST_AND_SET_IS_NEVER_NATIVE
#  define X_ATOMIC_INT_FETCH_AND_STORE_IS_NEVER_NATIVE
#  define X_ATOMIC_INT_FETCH_AND_ADD_IS_NEVER_NATIVE
#  define X_ATOMIC_INT32_REFERENCE_COUNTING_IS_NEVER_NATIVE
#  define X_ATOMIC_INT32_TEST_AND_SET_IS_NEVER_NATIVE
#  define X_ATOMIC_INT32_FETCH_AND_STORE_IS_NEVER_NATIVE
#  define X_ATOMIC_INT32_FETCH_AND_ADD_IS_NEVER_NATIVE

template <> inline bool XAtomicTraits<4>::isLockFree()
{ return false; }
#endif

#if ATOMIC_POINTER_LOCK_FREE == 2
#  define X_ATOMIC_POINTER_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
#  define X_ATOMIC_POINTER_TEST_AND_SET_IS_ALWAYS_NATIVE
#  define X_ATOMIC_POINTER_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#  define X_ATOMIC_POINTER_FETCH_AND_ADD_IS_ALWAYS_NATIVE
#elif ATOMIC_POINTER_LOCK_FREE == 1
#  define X_ATOMIC_POINTER_REFERENCE_COUNTING_IS_SOMETIMES_NATIVE
#  define X_ATOMIC_POINTER_TEST_AND_SET_IS_SOMETIMES_NATIVE
#  define X_ATOMIC_POINTER_FETCH_AND_STORE_IS_SOMETIMES_NATIVE
#  define X_ATOMIC_POINTER_FETCH_AND_ADD_IS_SOMETIMES_NATIVE
#else
#  define X_ATOMIC_POINTER_REFERENCE_COUNTING_IS_NEVER_NATIVE
#  define X_ATOMIC_POINTER_TEST_AND_SET_IS_NEVER_NATIVE
#  define X_ATOMIC_POINTER_FETCH_AND_STORE_IS_NEVER_NATIVE
#  define X_ATOMIC_POINTER_FETCH_AND_ADD_IS_NEVER_NATIVE
#endif

template<> struct XAtomicOpsSupport<1> { enum { IsSupported = 1 }; };
#define X_ATOMIC_INT8_IS_SUPPORTED
#if ATOMIC_CHAR_LOCK_FREE == 2
#  define X_ATOMIC_INT8_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
#  define X_ATOMIC_INT8_TEST_AND_SET_IS_ALWAYS_NATIVE
#  define X_ATOMIC_INT8_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#  define X_ATOMIC_INT8_FETCH_AND_ADD_IS_ALWAYS_NATIVE

template <> inline bool XAtomicTraits<1>::isLockFree()
{ return true; }
#elif ATOMIC_CHAR_LOCK_FREE == 1
#  define X_ATOMIC_INT8_REFERENCE_COUNTING_IS_SOMETIMES_NATIVE
#  define X_ATOMIC_INT8_TEST_AND_SET_IS_SOMETIMES_NATIVE
#  define X_ATOMIC_INT8_FETCH_AND_STORE_IS_SOMETIMES_NATIVE
#  define X_ATOMIC_INT8_FETCH_AND_ADD_IS_SOMETIMES_NATIVE

template <> inline bool XAtomicTraits<1>::isLockFree()
{ return false; }
#else
#  define X_ATOMIC_INT8_REFERENCE_COUNTING_IS_NEVER_NATIVE
#  define X_ATOMIC_INT8_TEST_AND_SET_IS_NEVER_NATIVE
#  define X_ATOMIC_INT8_FETCH_AND_STORE_IS_NEVER_NATIVE
#  define X_ATOMIC_INT8_FETCH_AND_ADD_IS_NEVER_NATIVE

template <> bool XAtomicTraits<1>::isLockFree()
{ return false; }
#endif

template<> struct XAtomicOpsSupport<2> { enum { IsSupported = 1 }; };
#define X_ATOMIC_INT16_IS_SUPPORTED
#if ATOMIC_SHORT_LOCK_FREE == 2
#  define X_ATOMIC_INT16_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
#  define X_ATOMIC_INT16_TEST_AND_SET_IS_ALWAYS_NATIVE
#  define X_ATOMIC_INT16_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#  define X_ATOMIC_INT16_FETCH_AND_ADD_IS_ALWAYS_NATIVE

template <> inline bool XAtomicTraits<2>::isLockFree()
{ return false; }
#elif ATOMIC_SHORT_LOCK_FREE == 1
#  define X_ATOMIC_INT16_REFERENCE_COUNTING_IS_SOMETIMES_NATIVE
#  define X_ATOMIC_INT16_TEST_AND_SET_IS_SOMETIMES_NATIVE
#  define X_ATOMIC_INT16_FETCH_AND_STORE_IS_SOMETIMES_NATIVE
#  define X_ATOMIC_INT16_FETCH_AND_ADD_IS_SOMETIMES_NATIVE

template <> inline bool XAtomicTraits<2>::isLockFree()
{ return false; }
#else
#  define X_ATOMIC_INT16_REFERENCE_COUNTING_IS_NEVER_NATIVE
#  define X_ATOMIC_INT16_TEST_AND_SET_IS_NEVER_NATIVE
#  define X_ATOMIC_INT16_FETCH_AND_STORE_IS_NEVER_NATIVE
#  define X_ATOMIC_INT16_FETCH_AND_ADD_IS_NEVER_NATIVE

template <> inline bool XAtomicTraits<2>::isLockFree()
{ return false; }
#endif

#if !defined(X_BOOTSTRAPPED)
template<> struct XAtomicOpsSupport<8> { enum { IsSupported = 1 }; };
#  define X_ATOMIC_INT64_IS_SUPPORTED
#  if ATOMIC_LLONG_LOCK_FREE == 2
#    define X_ATOMIC_INT64_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
#    define X_ATOMIC_INT64_TEST_AND_SET_IS_ALWAYS_NATIVE
#    define X_ATOMIC_INT64_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#    define X_ATOMIC_INT64_FETCH_AND_ADD_IS_ALWAYS_NATIVE

template <> inline bool XAtomicTraits<8>::isLockFree()
{ return true; }
#  elif ATOMIC_LLONG_LOCK_FREE == 1
#    define X_ATOMIC_INT64_REFERENCE_COUNTING_IS_SOMETIMES_NATIVE
#    define X_ATOMIC_INT64_TEST_AND_SET_IS_SOMETIMES_NATIVE
#    define X_ATOMIC_INT64_FETCH_AND_STORE_IS_SOMETIMES_NATIVE
#    define X_ATOMIC_INT64_FETCH_AND_ADD_IS_SOMETIMES_NATIVE

template <> inline bool XAtomicTraits<8>::isLockFree()
{ return false; }
#  else
#    define X_ATOMIC_INT64_REFERENCE_COUNTING_IS_NEVER_NATIVE
#    define X_ATOMIC_INT64_TEST_AND_SET_IS_NEVER_NATIVE
#    define X_ATOMIC_INT64_FETCH_AND_STORE_IS_NEVER_NATIVE
#    define X_ATOMIC_INT64_FETCH_AND_ADD_IS_NEVER_NATIVE

template <> inline bool XAtomicTraits<8>::isLockFree()
{ return false; }
#  endif
#endif

template <typename X> class XAtomicOpsBase {
public:
    using Type = std::atomic<X>;

    template <typename T> static inline
    T load(const std::atomic<T> &_x_value) noexcept{
        return _x_value.load(std::memory_order_relaxed);
    }

    template <typename T> static inline
    T load(const volatile std::atomic<T> &_x_value) noexcept{
        return _x_value.load(std::memory_order_relaxed);
    }

    template <typename T> static inline
    T loadRelaxed(const std::atomic<T> &_x_value) noexcept{
        return _x_value.load(std::memory_order_relaxed);
    }

    template <typename T> static inline
    T loadRelaxed(const volatile std::atomic<T> &_x_value) noexcept{
        return _x_value.load(std::memory_order_relaxed);
    }

    template <typename T> static inline
    T loadAcquire(const std::atomic<T> &_x_value) noexcept{
        return _x_value.load(std::memory_order_acquire);
    }

    template <typename T> static inline
    T loadAcquire(const volatile std::atomic<T> &_x_value) noexcept{
        return _x_value.load(std::memory_order_acquire);
    }

    template <typename T> static inline
    void store(std::atomic<T> &_x_value,const T &newValue) noexcept{
        _x_value.store(newValue, std::memory_order_relaxed);
    }

    template <typename T> static inline
    void storeRelaxed(std::atomic<T> &_x_value,const T &newValue) noexcept{
        _x_value.store(newValue, std::memory_order_relaxed);
    }

    template <typename T> static inline
    void storeRelease(std::atomic<T> &_x_value,const T &newValue) noexcept{
        _x_value.store(newValue, std::memory_order_release);
    }

    static inline bool isReferenceCountingNative() noexcept { return isTestAndSetNative(); }

    static inline constexpr bool isReferenceCountingWaitFree() noexcept { return false; }

    static inline bool isTestAndSetNative() noexcept
    { return XAtomicTraits<sizeof(X)>::isLockFree(); }

    static inline constexpr bool isTestAndSetWaitFree() noexcept { return false; }

    template <typename T>
    static bool testAndSetRelaxed(std::atomic<T> &_x_value, T expectedValue,
        const T &newValue, T *currentValue = nullptr) noexcept{

        const auto b{_x_value.compare_exchange_strong(expectedValue, newValue,
                                                    std::memory_order_relaxed, std::memory_order_relaxed)};
        if (currentValue){
            *currentValue = expectedValue;
        }
        return b;
    }

    template <typename T>
    static bool testAndSetAcquire(std::atomic<T> &_x_value, T expectedValue,
        const T &newValue, T *currentValue = nullptr) noexcept{
        const auto b{_x_value.compare_exchange_strong(expectedValue, newValue,
            std::memory_order_acquire, std::memory_order_acquire)};
        if (currentValue){
            *currentValue = expectedValue;
        }
        return b;
    }

    template <typename T>
    static bool testAndSetRelease(std::atomic<T> &_x_value,T expectedValue,
        const T &newValue, T *currentValue = nullptr) noexcept{
        const auto b{_x_value.compare_exchange_strong(expectedValue, newValue,
            std::memory_order_release,std::memory_order_relaxed)};
        if (currentValue){
            *currentValue = expectedValue;
        }
        return b;
    }

    template <typename T>
    static bool testAndSetOrdered(std::atomic<T> &_x_value, T expectedValue,
        const T &newValue, T *currentValue = nullptr) noexcept{
        const auto b{_x_value.compare_exchange_strong(expectedValue,newValue,
            std::memory_order_acq_rel, std::memory_order_acquire)};
        if (currentValue){
            *currentValue = expectedValue;
        }
        return b;
    }

    static inline bool isFetchAndStoreNative() noexcept { return isTestAndSetNative(); }
    static inline constexpr bool isFetchAndStoreWaitFree() noexcept { return false; }
};

template <typename X> class XAtomicOps final : public XAtomicOpsBase<X> {
    using Base_ = XAtomicOpsBase<X>;
public:
    template <typename T>
    static inline bool ref(std::atomic<T> &_x_value){
        /* Conceptually, we want to
         *    return ++_x_value != 0;
         * However, that would be sequentially consistent, and thus stronger
         * than what we need. Based on
         * http://eel.is/c++draft/atomics.types.memop#6, we know that
         * pre-increment is equivalent to fetch_add(1) + 1. Unlike
         * pre-increment, fetch_add takes a memory order argument, so we can get
         * the desired acquire-release semantics.
         * One last gotcha is that fetch_add(1) + 1 would need to be converted
         * back to T, because it's susceptible to integer promotion. To sidestep
         * this issue and to avoid UB on signed overflow, we rewrite the
         * expression to:
         */
        return _x_value.fetch_add(1, std::memory_order_acq_rel) != T(-1);
    }

    template <typename T>
    static inline bool deref(std::atomic<T> &_x_value) noexcept{
        // compare with ref
        return _x_value.fetch_sub(1, std::memory_order_acq_rel) != T(1);
    }

    template <typename T>
    static T fetchAndStoreRelaxed(std::atomic<T> &_x_value,const T &newValue) noexcept{
        return _x_value.exchange(newValue,std::memory_order_relaxed);
    }

    template <typename T>
    static T fetchAndStoreAcquire(std::atomic<T> &_x_value,const T &newValue) noexcept{
        return _x_value.exchange(newValue, std::memory_order_acquire);
    }

    template <typename T>
    static T fetchAndStoreRelease(std::atomic<T> &_x_value,const T &newValue) noexcept{
        return _x_value.exchange(newValue,std::memory_order_release);
    }

    template <typename T>
    static T fetchAndStoreOrdered(std::atomic<T> &_x_value,const T &newValue) noexcept{
        return _x_value.exchange(newValue, std::memory_order_acq_rel);
    }

    static inline bool isFetchAndAddNative() noexcept { return Base_::isTestAndSetNative(); }
    static inline constexpr bool isFetchAndAddWaitFree() noexcept { return false; }

    template <typename T> static inline
    T fetchAndAddRelaxed(std::atomic<T> &_x_value,
        const typename XAtomicAdditiveType<T>::AdditiveT &valueToAdd) noexcept{
        return _x_value.fetch_add(valueToAdd, std::memory_order_relaxed);
    }

    template <typename T> static inline
    T fetchAndAddAcquire(std::atomic<T> &_x_value,const typename XAtomicAdditiveType<T>::AdditiveT &valueToAdd) noexcept{
        return _x_value.fetch_add(valueToAdd, std::memory_order_acquire);
    }

    template <typename T> static inline
    T fetchAndAddRelease(std::atomic<T> &_x_value,const typename XAtomicAdditiveType<T>::AdditiveT &valueToAdd) noexcept{
        return _x_value.fetch_add(valueToAdd, std::memory_order_release);
    }

    template <typename T> static inline
    T fetchAndAddOrdered(std::atomic<T> &_x_value,const typename XAtomicAdditiveType<T>::AdditiveT &valueToAdd) noexcept{
        return _x_value.fetch_add(valueToAdd, std::memory_order_acq_rel);
    }

    template <typename T> static inline
    T fetchAndSubRelaxed(std::atomic<T> &_x_value,const typename XAtomicAdditiveType<T>::AdditiveT &valueToAdd) noexcept{
        return _x_value.fetch_sub(valueToAdd, std::memory_order_relaxed);
    }

    template <typename T> static inline
    T fetchAndSubAcquire(std::atomic<T> &_x_value,const typename XAtomicAdditiveType<T>::AdditiveT &valueToAdd) noexcept{
        return _x_value.fetch_sub(valueToAdd, std::memory_order_acquire);
    }

    template <typename T> static inline
    T fetchAndSubRelease(std::atomic<T> &_x_value,const typename XAtomicAdditiveType<T>::AdditiveT &valueToAdd) noexcept{
        return _x_value.fetch_sub(valueToAdd, std::memory_order_release);
    }

    template <typename T> static inline
    T fetchAndSubOrdered(std::atomic<T> &_x_value,const typename XAtomicAdditiveType<T>::AdditiveT &valueToAdd) noexcept{
        return _x_value.fetch_sub(valueToAdd, std::memory_order_acq_rel);
    }

    template <typename T> static inline
    T fetchAndAndRelaxed(std::atomic<T> &_x_value,const typename XAtomicAdditiveType<T>::AdditiveT &valueToAdd) noexcept{
        return _x_value.fetch_and(valueToAdd, std::memory_order_relaxed);
    }

    template <typename T> static inline
    T fetchAndAndAcquire(std::atomic<T> &_x_value,const typename XAtomicAdditiveType<T>::AdditiveT &valueToAdd) noexcept{
        return _x_value.fetch_and(valueToAdd, std::memory_order_acquire);
    }

    template <typename T> static inline
    T fetchAndAndRelease(std::atomic<T> &_x_value,const typename XAtomicAdditiveType<T>::AdditiveT &valueToAdd) noexcept{
        return _x_value.fetch_and(valueToAdd, std::memory_order_release);
    }

    template <typename T> static inline
    T fetchAndAndOrdered(std::atomic<T> &_x_value,const typename XAtomicAdditiveType<T>::AdditiveT &valueToAdd) noexcept{
        return _x_value.fetch_and(valueToAdd, std::memory_order_acq_rel);
    }

    template <typename T> static inline
    T fetchAndOrRelaxed(std::atomic<T> &_x_value,const typename XAtomicAdditiveType<T>::AdditiveT &valueToAdd) noexcept{
        return _x_value.fetch_or(valueToAdd, std::memory_order_relaxed);
    }

    template <typename T> static inline
    T fetchAndOrAcquire(std::atomic<T> &_x_value,const typename XAtomicAdditiveType<T>::AdditiveT &valueToAdd) noexcept{
        return _x_value.fetch_or(valueToAdd, std::memory_order_acquire);
    }

    template <typename T> static inline
    T fetchAndOrRelease(std::atomic<T> &_x_value,const typename XAtomicAdditiveType<T>::AdditiveT &valueToAdd) noexcept{
        return _x_value.fetch_or(valueToAdd, std::memory_order_release);
    }

    template <typename T> static inline
    T fetchAndOrOrdered(std::atomic<T> &_x_value,const typename XAtomicAdditiveType<T>::AdditiveT &valueToAdd) noexcept{
        return _x_value.fetch_or(valueToAdd, std::memory_order_acq_rel);
    }

    template <typename T> static inline
    T fetchAndXorRelaxed(std::atomic<T> &_x_value,const typename XAtomicAdditiveType<T>::AdditiveT &valueToAdd) noexcept{
        return _x_value.fetch_xor(valueToAdd, std::memory_order_relaxed);
    }

    template <typename T> static inline
    T fetchAndXorAcquire(std::atomic<T> &_x_value,const typename XAtomicAdditiveType<T>::AdditiveT &valueToAdd) noexcept{
        return _x_value.fetch_xor(valueToAdd, std::memory_order_acquire);
    }

    template <typename T> static inline
    T fetchAndXorRelease(std::atomic<T> &_x_value,const typename XAtomicAdditiveType<T>::AdditiveT &valueToAdd) noexcept{
        return _x_value.fetch_xor(valueToAdd, std::memory_order_release);
    }

    template <typename T> static inline
    T fetchAndXorOrdered(std::atomic<T> &_x_value,const typename XAtomicAdditiveType<T>::AdditiveT &valueToAdd) noexcept{
        return _x_value.fetch_xor(valueToAdd, std::memory_order_acq_rel);
    }
};

#define Q_BASIC_ATOMIC_INITIALIZER(a)     { a }

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
