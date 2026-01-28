#ifndef X_BASIC_ATOMIC_HPP
#define X_BASIC_ATOMIC_HPP 1

#include <XAtomic/xatomic_cxx11.hpp>
#include <XGlobal/xclasshelpermacros.hpp>
#include <XGlobal/xversion.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template <typename T>
class XBasicAtomic {
public:
    using Type = T;
    using Ops = std::conditional_t<std::is_same_v<Type,bool>,XAtomicOpsBase<T>,XAtomicOps<T>>;
    using AtomicType = Ops::Type;
    AtomicType m_x_value {};

    Type loadRelaxed() const noexcept { return Ops::loadRelaxed(m_x_value); }
    void storeRelaxed(Type const & newValue) noexcept { Ops::storeRelaxed(m_x_value, newValue); }

    Type loadAcquire() const noexcept { return Ops::loadAcquire(m_x_value); }
    void storeRelease(Type const & newValue) noexcept { Ops::storeRelease(m_x_value, newValue); }
    X_IMPLICIT operator Type() const noexcept { return loadAcquire(); }
    Type operator=(Type const & newValue) noexcept { storeRelease(newValue); return newValue; }

    static constexpr bool isTestAndSetNative() noexcept { return Ops::isTestAndSetNative(); }
    static constexpr bool isTestAndSetWaitFree() noexcept { return Ops::isTestAndSetWaitFree(); }

    bool testAndSetRelaxed(T const & expectedValue, T const & newValue) noexcept
    { return Ops::testAndSetRelaxed(m_x_value, expectedValue, newValue); }
    bool testAndSetAcquire(T const & expectedValue, T const & newValue) noexcept
    { return Ops::testAndSetAcquire(m_x_value, expectedValue, newValue); }
    bool testAndSetRelease(T const & expectedValue, T const & newValue) noexcept
    { return Ops::testAndSetRelease(m_x_value, expectedValue, newValue); }
    bool testAndSetOrdered(T const & expectedValue, T const & newValue) noexcept
    { return Ops::testAndSetOrdered(m_x_value, expectedValue, newValue); }

    bool testAndSetRelaxed(T const & expectedValue, T const & newValue, T & currentValue) noexcept
    { return Ops::testAndSetRelaxed(m_x_value, expectedValue, newValue, &currentValue); }
    bool testAndSetAcquire(T const & expectedValue, T const & newValue, T & currentValue) noexcept
    { return Ops::testAndSetAcquire(m_x_value, expectedValue, newValue, &currentValue); }
    bool testAndSetRelease(T const & expectedValue, T const & newValue, T & currentValue) noexcept
    { return Ops::testAndSetRelease(m_x_value, expectedValue, newValue, &currentValue); }
    bool testAndSetOrdered(T const & expectedValue, T const & newValue, T & currentValue) noexcept
    { return Ops::testAndSetOrdered(m_x_value, expectedValue, newValue, &currentValue); }

    static constexpr bool isFetchAndStoreNative() noexcept { return Ops::isFetchAndStoreNative(); }
    static constexpr bool isFetchAndStoreWaitFree() noexcept { return Ops::isFetchAndStoreWaitFree(); }

    T fetchAndStoreRelaxed(T const & newValue) noexcept
    { return Ops::fetchAndStoreRelaxed(m_x_value, newValue); }
    T fetchAndStoreAcquire(T const & newValue) noexcept
    { return Ops::fetchAndStoreAcquire(m_x_value, newValue); }
    T fetchAndStoreRelease(T const & newValue) noexcept
    { return Ops::fetchAndStoreRelease(m_x_value, newValue); }
    T fetchAndStoreOrdered(T const & newValue) noexcept
    { return Ops::fetchAndStoreOrdered(m_x_value, newValue); }

    X_IMPLICIT constexpr XBasicAtomic(T const & value = {}) noexcept
        : m_x_value {value} { }
    XBasicAtomic(XBasicAtomic const &) = delete;
    XBasicAtomic &operator=(XBasicAtomic const &) = delete;
    XBasicAtomic &operator=(XBasicAtomic const &) volatile = delete;
};

template <typename T>
class XBasicAtomicInteger : public XBasicAtomic<T> {
    // static check that this is a valid integer
    static_assert(std::is_integral_v<T>, "template parameter is not an integral type");
    static_assert(XAtomicOpsSupport<sizeof(T)>::IsSupported, "template parameter is an integral of a size not supported on this platform");
    using Base_ = XBasicAtomic<T>;
    using Ops = Base_::Ops;

public:
    bool ref() noexcept { return Ops::ref(this->m_x_value); }
    bool deref() noexcept { return Ops::deref(this->m_x_value); }

    static constexpr bool isFetchAndAddNative() noexcept { return Ops::isFetchAndAddNative(); }
    static constexpr bool isFetchAndAddWaitFree() noexcept { return Ops::isFetchAndAddWaitFree(); }

    T fetchAndAddRelaxed(T const & valueToAdd) noexcept
    { return Base_::Ops::fetchAndAddRelaxed(this->m_x_value, valueToAdd); }
    T fetchAndAddAcquire(T const & valueToAdd) noexcept
    { return Base_::Ops::fetchAndAddAcquire(this->m_x_value, valueToAdd); }
    T fetchAndAddRelease(T const & valueToAdd) noexcept
    { return Ops::fetchAndAddRelease(this->m_x_value, valueToAdd); }
    T fetchAndAddOrdered(T const & valueToAdd) noexcept
    { return Ops::fetchAndAddOrdered(this->m_x_value, valueToAdd); }

    T fetchAndSubRelaxed(T const & valueToAdd) noexcept
    { return Ops::fetchAndSubRelaxed(this->m_x_value, valueToAdd); }
    T fetchAndSubAcquire(T const & valueToAdd) noexcept
    { return Ops::fetchAndSubAcquire(this->m_x_value, valueToAdd); }
    T fetchAndSubRelease(T const & valueToAdd) noexcept
    { return Ops::fetchAndSubRelease(this->m_x_value, valueToAdd); }
    T fetchAndSubOrdered(T const & valueToAdd) noexcept
    { return Ops::fetchAndSubOrdered(this->m_x_value, valueToAdd); }

    T fetchAndAndRelaxed(T const & valueToAdd) noexcept
    { return Ops::fetchAndAndRelaxed(this->m_x_value, valueToAdd); }
    T fetchAndAndAcquire(T const & valueToAdd) noexcept
    { return Ops::fetchAndAndAcquire(this->m_x_value, valueToAdd); }
    T fetchAndAndRelease(T const & valueToAdd) noexcept
    { return Ops::fetchAndAndRelease(this->m_x_value, valueToAdd); }
    T fetchAndAndOrdered(T const & valueToAdd) noexcept
    { return Ops::fetchAndAndOrdered(this->m_x_value, valueToAdd); }

    T fetchAndOrRelaxed(T const & valueToAdd) noexcept
    { return Ops::fetchAndOrRelaxed(this->m_x_value, valueToAdd); }
    T fetchAndOrAcquire(T const & valueToAdd) noexcept
    { return Ops::fetchAndOrAcquire(this->m_x_value, valueToAdd); }
    T fetchAndOrRelease(T const & valueToAdd) noexcept
    { return Ops::fetchAndOrRelease(this->m_x_value, valueToAdd); }
    T fetchAndOrOrdered(T const & valueToAdd) noexcept
    { return Ops::fetchAndOrOrdered(this->m_x_value, valueToAdd); }

    T fetchAndXorRelaxed(T const & valueToAdd) noexcept
    { return Ops::fetchAndXorRelaxed(this->m_x_value, valueToAdd); }
    T fetchAndXorAcquire(T const & valueToAdd) noexcept
    { return Ops::fetchAndXorAcquire(this->m_x_value, valueToAdd); }
    T fetchAndXorRelease(T const & valueToAdd) noexcept
    { return Ops::fetchAndXorRelease(this->m_x_value, valueToAdd); }
    T fetchAndXorOrdered(T const & valueToAdd) noexcept
    { return Ops::fetchAndXorOrdered(this->m_x_value, valueToAdd); }

    T operator++() noexcept
    { return fetchAndAddOrdered(1) + 1; }
    T operator++(int) noexcept
    { return fetchAndAddOrdered(1); }
    T operator--() noexcept
    { return fetchAndSubOrdered(1) - 1; }
    T operator--(int) noexcept
    { return fetchAndSubOrdered(1); }

    T operator+=(T const & v) noexcept
    { return fetchAndAddOrdered(v) + v; }
    T operator-=(T const & v) noexcept
    { return fetchAndSubOrdered(v) - v; }
    T operator&=(T const & v) noexcept
    { return fetchAndAndOrdered(v) & v; }
    T operator|=(T const& v) noexcept
    { return fetchAndOrOrdered(v) | v; }
    T operator^=(T const & v) noexcept
    { return fetchAndXorOrdered(v) ^ v; }

    constexpr XBasicAtomicInteger() = default;
    X_IMPLICIT constexpr XBasicAtomicInteger(T const & value ) noexcept
        : Base_ {value} { }
    XBasicAtomicInteger(XBasicAtomicInteger const &) = delete;
    XBasicAtomicInteger &operator=(XBasicAtomicInteger const &) = delete;
    XBasicAtomicInteger &operator=(XBasicAtomicInteger const &) volatile = delete;
};

using XBasicAtomicInt [[maybe_unused]] = XBasicAtomicInteger<int>;

template <typename X>
class XBasicAtomicPointer {
public:
    using Type = X*;
    using Ops = XAtomicOps<Type>;
    using AtomicType = Ops::Type;
    AtomicType m_x_value;

    Type loadRelaxed() const noexcept { return Ops::loadRelaxed(m_x_value); }
    void storeRelaxed(Type const & newValue) noexcept { Ops::storeRelaxed(m_x_value, newValue); }

    X_IMPLICIT operator Type() const noexcept { return loadAcquire(); }
    Type operator=(Type const & newValue) noexcept { storeRelease(newValue); return newValue; }

    // Atomic API, implemented in qatomic_XXX.h
    Type loadAcquire() const noexcept { return Ops::loadAcquire(m_x_value); }
    void storeRelease(Type const & newValue) noexcept { Ops::storeRelease(m_x_value, newValue); }

    static constexpr bool isTestAndSetNative() noexcept { return Ops::isTestAndSetNative(); }
    static constexpr bool isTestAndSetWaitFree() noexcept { return Ops::isTestAndSetWaitFree(); }

    bool testAndSetRelaxed(Type const & expectedValue,Type const & newValue) noexcept
    { return Ops::testAndSetRelaxed(m_x_value, expectedValue, newValue); }
    bool testAndSetAcquire(Type const & expectedValue,Type const & newValue) noexcept
    { return Ops::testAndSetAcquire(m_x_value, expectedValue, newValue); }
    bool testAndSetRelease(Type const & expectedValue,Type const & newValue) noexcept
    { return Ops::testAndSetRelease(m_x_value, expectedValue, newValue); }
    bool testAndSetOrdered(Type const & expectedValue,Type const & newValue) noexcept
    { return Ops::testAndSetOrdered(m_x_value, expectedValue, newValue); }

    bool testAndSetRelaxed(Type const & expectedValue,Type const & newValue, Type & currentValue) noexcept
    { return Ops::testAndSetRelaxed(m_x_value, expectedValue, newValue, &currentValue); }
    bool testAndSetAcquire(Type const & expectedValue,Type const & newValue, Type & currentValue) noexcept
    { return Ops::testAndSetAcquire(m_x_value, expectedValue, newValue, &currentValue); }
    bool testAndSetRelease(Type const & expectedValue,Type const & newValue, Type & currentValue) noexcept
    { return Ops::testAndSetRelease(m_x_value, expectedValue, newValue, &currentValue); }
    bool testAndSetOrdered(Type const & expectedValue,Type const & newValue, Type & currentValue) noexcept
    { return Ops::testAndSetOrdered(m_x_value, expectedValue, newValue, &currentValue); }

    static constexpr bool isFetchAndStoreNative() noexcept { return Ops::isFetchAndStoreNative(); }
    static constexpr bool isFetchAndStoreWaitFree() noexcept { return Ops::isFetchAndStoreWaitFree(); }

    Type fetchAndStoreRelaxed(Type const & newValue) noexcept
    { return Ops::fetchAndStoreRelaxed(m_x_value, newValue); }
    Type fetchAndStoreAcquire(Type const & newValue) noexcept
    { return Ops::fetchAndStoreAcquire(m_x_value, newValue); }
    Type fetchAndStoreRelease(Type const & newValue) noexcept
    { return Ops::fetchAndStoreRelease(m_x_value, newValue); }
    Type fetchAndStoreOrdered(Type const & newValue) noexcept
    { return Ops::fetchAndStoreOrdered(m_x_value, newValue); }

    static constexpr bool isFetchAndAddNative() noexcept { return Ops::isFetchAndAddNative(); }
    static constexpr bool isFetchAndAddWaitFree() noexcept { return Ops::isFetchAndAddWaitFree(); }

    Type fetchAndAddRelaxed(xptrdiff const & valueToAdd) noexcept
    { return Ops::fetchAndAddRelaxed(m_x_value, valueToAdd); }
    Type fetchAndAddAcquire(xptrdiff const & valueToAdd) noexcept
    { return Ops::fetchAndAddAcquire(m_x_value, valueToAdd); }
    Type fetchAndAddRelease(xptrdiff const & valueToAdd) noexcept
    { return Ops::fetchAndAddRelease(m_x_value, valueToAdd); }
    Type fetchAndAddOrdered(xptrdiff const & valueToAdd) noexcept
    { return Ops::fetchAndAddOrdered(m_x_value, valueToAdd); }

    Type fetchAndSubRelaxed(xptrdiff const & valueToSub) noexcept
    { return Ops::fetchAndSubRelaxed(m_x_value, valueToSub); }
    Type fetchAndSubAcquire(xptrdiff const & valueToSub) noexcept
    { return Ops::fetchAndSubAcquire(m_x_value, valueToSub); }
    Type fetchAndSubRelease(xptrdiff const & valueToSub) noexcept
    { return Ops::fetchAndSubRelease(m_x_value, valueToSub); }
    Type fetchAndSubOrdered(xptrdiff const & valueToSub) noexcept
    { return Ops::fetchAndSubOrdered(m_x_value, valueToSub); }

    Type operator++() noexcept
    { return fetchAndAddOrdered(1) + 1; }
    Type operator++(int) noexcept
    { return fetchAndAddOrdered(1); }
    Type operator--() noexcept
    { return fetchAndSubOrdered(1) - 1; }
    Type operator--(int) noexcept
    { return fetchAndSubOrdered(1); }
    Type operator+=(const xptrdiff &valueToAdd) noexcept
    { return fetchAndAddOrdered(valueToAdd) + valueToAdd; }
    Type operator-=(const xptrdiff &valueToSub) noexcept
    { return fetchAndSubOrdered(valueToSub) - valueToSub; }

    constexpr XBasicAtomicPointer() = default;
    X_IMPLICIT constexpr XBasicAtomicPointer(Type const & value) noexcept
        : m_x_value {value} { }
    XBasicAtomicPointer(XBasicAtomicPointer const &) = delete;
    XBasicAtomicPointer &operator=(XBasicAtomicPointer const &) = delete;
    XBasicAtomicPointer &operator=(XBasicAtomicPointer const &) volatile = delete;
};

#ifndef X_BASIC_ATOMIC_INITIALIZER
#  define X_BASIC_ATOMIC_INITIALIZER(a) { (a) }
#endif

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
