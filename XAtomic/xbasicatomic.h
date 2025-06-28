#ifndef X_BASIC_ATOMIC_HPP
#define X_BASIC_ATOMIC_HPP 1

#include "xatomic_cxx11.hpp"

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template <typename T>
class XBasicAtomic {
public:
    using Type = T;
    using Ops = std::conditional_t<std::is_same_v<Type,bool>,XAtomicOpsBase<T>,XAtomicOps<T>>;
    using AtomicType = typename Ops::Type;
    AtomicType m_x_value{};

    Type loadRelaxed() const noexcept { return Ops::loadRelaxed(m_x_value); }
    void storeRelaxed(const Type &newValue) noexcept { Ops::storeRelaxed(m_x_value, newValue); }

    Type loadAcquire() const noexcept { return Ops::loadAcquire(m_x_value); }
    void storeRelease(const Type &newValue) noexcept { Ops::storeRelease(m_x_value, newValue); }
    explicit operator Type() const noexcept { return loadAcquire(); }
    [[maybe_unused]] Type operator=(const Type &newValue) noexcept { storeRelease(newValue); return newValue; }

    static constexpr bool isTestAndSetNative() noexcept { return Ops::isTestAndSetNative(); }
    static constexpr bool isTestAndSetWaitFree() noexcept { return Ops::isTestAndSetWaitFree(); }

    bool testAndSetRelaxed(const T &expectedValue, const T &newValue) noexcept
    { return Ops::testAndSetRelaxed(m_x_value, expectedValue, newValue); }
    bool testAndSetAcquire(const T &expectedValue, const T &newValue) noexcept
    { return Ops::testAndSetAcquire(m_x_value, expectedValue, newValue); }
    bool testAndSetRelease(const T &expectedValue, const T &newValue) noexcept
    { return Ops::testAndSetRelease(m_x_value, expectedValue, newValue); }
    bool testAndSetOrdered(const T &expectedValue, const T &newValue) noexcept
    { return Ops::testAndSetOrdered(m_x_value, expectedValue, newValue); }

    bool testAndSetRelaxed(const T &expectedValue, const T &newValue, T &currentValue) noexcept
    { return Ops::testAndSetRelaxed(m_x_value, expectedValue, newValue, &currentValue); }
    bool testAndSetAcquire(const T &expectedValue, const T &newValue, T &currentValue) noexcept
    { return Ops::testAndSetAcquire(m_x_value, expectedValue, newValue, &currentValue); }
    bool testAndSetRelease(const T &expectedValue, const T &newValue, T &currentValue) noexcept
    { return Ops::testAndSetRelease(m_x_value, expectedValue, newValue, &currentValue); }
    bool testAndSetOrdered(const T &expectedValue, const T &newValue, T &currentValue) noexcept
    { return Ops::testAndSetOrdered(m_x_value, expectedValue, newValue, &currentValue); }

    static constexpr bool isFetchAndStoreNative() noexcept { return Ops::isFetchAndStoreNative(); }
    static constexpr bool isFetchAndStoreWaitFree() noexcept { return Ops::isFetchAndStoreWaitFree(); }

    T fetchAndStoreRelaxed(const T &newValue) noexcept
    { return Ops::fetchAndStoreRelaxed(m_x_value, newValue); }
    T fetchAndStoreAcquire(const T &newValue) noexcept
    { return Ops::fetchAndStoreAcquire(m_x_value, newValue); }
    T fetchAndStoreRelease(const T &newValue) noexcept
    { return Ops::fetchAndStoreRelease(m_x_value, newValue); }
    T fetchAndStoreOrdered(const T &newValue) noexcept
    { return Ops::fetchAndStoreOrdered(m_x_value, newValue); }

    explicit constexpr XBasicAtomic(const T &value = T{}) noexcept : m_x_value(value) {}
    XBasicAtomic(const XBasicAtomic &) = delete;
    XBasicAtomic &operator=(const XBasicAtomic &) = delete;
    XBasicAtomic &operator=(const XBasicAtomic &) volatile = delete;
};

template <typename T>
class XBasicAtomicInteger : public XBasicAtomic<T> {
    // static check that this is a valid integer
    static_assert(std::is_integral_v<T>, "template parameter is not an integral type");
    static_assert(XAtomicOpsSupport<sizeof(T)>::IsSupported, "template parameter is an integral of a size not supported on this platform");
    using Base_ = XBasicAtomic<T>;
    using Ops = typename Base_::Ops;
public:
    bool ref() noexcept { return Ops::ref(this->m_x_value); }
    bool deref() noexcept { return Ops::deref(this->m_x_value); }

    static constexpr bool isFetchAndAddNative() noexcept { return Ops::isFetchAndAddNative(); }
    static constexpr bool isFetchAndAddWaitFree() noexcept { return Ops::isFetchAndAddWaitFree(); }

    T fetchAndAddRelaxed(const T &valueToAdd) noexcept
    { return Base_::Ops::fetchAndAddRelaxed(this->m_x_value, valueToAdd); }
    T fetchAndAddAcquire(const T &valueToAdd) noexcept
    { return Base_::Ops::fetchAndAddAcquire(this->m_x_value, valueToAdd); }
    T fetchAndAddRelease(const T &valueToAdd) noexcept
    { return Ops::fetchAndAddRelease(this->m_x_value, valueToAdd); }
    T fetchAndAddOrdered(const T &valueToAdd) noexcept
    { return Ops::fetchAndAddOrdered(this->m_x_value, valueToAdd); }

    T fetchAndSubRelaxed(const T &valueToAdd) noexcept
    { return Ops::fetchAndSubRelaxed(this->m_x_value, valueToAdd); }
    T fetchAndSubAcquire(const T &valueToAdd) noexcept
    { return Ops::fetchAndSubAcquire(this->m_x_value, valueToAdd); }
    T fetchAndSubRelease(const T &valueToAdd) noexcept
    { return Ops::fetchAndSubRelease(this->m_x_value, valueToAdd); }
    T fetchAndSubOrdered(const T &valueToAdd) noexcept
    { return Ops::fetchAndSubOrdered(this->m_x_value, valueToAdd); }

    T fetchAndAndRelaxed(const T &valueToAdd) noexcept
    { return Ops::fetchAndAndRelaxed(this->m_x_value, valueToAdd); }
    T fetchAndAndAcquire(const T &valueToAdd) noexcept
    { return Ops::fetchAndAndAcquire(this->m_x_value, valueToAdd); }
    T fetchAndAndRelease(const T &valueToAdd) noexcept
    { return Ops::fetchAndAndRelease(this->m_x_value, valueToAdd); }
    T fetchAndAndOrdered(const T &valueToAdd) noexcept
    { return Ops::fetchAndAndOrdered(this->m_x_value, valueToAdd); }

    T fetchAndOrRelaxed(const T &valueToAdd) noexcept
    { return Ops::fetchAndOrRelaxed(this->m_x_value, valueToAdd); }
    T fetchAndOrAcquire(const T &valueToAdd) noexcept
    { return Ops::fetchAndOrAcquire(this->m_x_value, valueToAdd); }
    T fetchAndOrRelease(const T &valueToAdd) noexcept
    { return Ops::fetchAndOrRelease(this->m_x_value, valueToAdd); }
    T fetchAndOrOrdered(const T &valueToAdd) noexcept
    { return Ops::fetchAndOrOrdered(this->m_x_value, valueToAdd); }

    T fetchAndXorRelaxed(const T &valueToAdd) noexcept
    { return Ops::fetchAndXorRelaxed(this->m_x_value, valueToAdd); }
    T fetchAndXorAcquire(const T &valueToAdd) noexcept
    { return Ops::fetchAndXorAcquire(this->m_x_value, valueToAdd); }
    T fetchAndXorRelease(const T &valueToAdd) noexcept
    { return Ops::fetchAndXorRelease(this->m_x_value, valueToAdd); }
    T fetchAndXorOrdered(const T &valueToAdd) noexcept
    { return Ops::fetchAndXorOrdered(this->m_x_value, valueToAdd); }

    T operator++() noexcept
    { return fetchAndAddOrdered(1) + 1; }
    T operator++(int) noexcept
    { return fetchAndAddOrdered(1); }
    T operator--() noexcept
    { return fetchAndSubOrdered(1) - 1; }
    T operator--(int) noexcept
    { return fetchAndSubOrdered(1); }

    T operator+=(const T &v) noexcept
    { return fetchAndAddOrdered(v) + v; }
    T operator-=(const T &v) noexcept
    { return fetchAndSubOrdered(v) - v; }
    T operator&=(const T &v) noexcept
    { return fetchAndAndOrdered(v) & v; }
    T operator|=(const T &v) noexcept
    { return fetchAndOrOrdered(v) | v; }
    T operator^=(const T &v) noexcept
    { return fetchAndXorOrdered(v) ^ v; }

    explicit constexpr XBasicAtomicInteger(const T &value = T{}) noexcept : Base_(value) {}
    XBasicAtomicInteger(const XBasicAtomicInteger &) = delete;
    XBasicAtomicInteger &operator=(const XBasicAtomicInteger &) = delete;
    XBasicAtomicInteger &operator=(const XBasicAtomicInteger &) volatile = delete;
};

using XBasicAtomicInt [[maybe_unused]] = XBasicAtomicInteger<int>;

template <typename X>
class XBasicAtomicPointer{
public:
    using Type = X*;
    using Ops = XAtomicOps<Type>;
    using AtomicType = typename Ops::Type;
    AtomicType m_x_value;

    Type loadRelaxed() const noexcept { return Ops::loadRelaxed(m_x_value); }
    void storeRelaxed(const Type &newValue) noexcept { Ops::storeRelaxed(m_x_value, newValue); }

    explicit operator Type() const noexcept { return loadAcquire(); }
    [[maybe_unused]] Type operator=(const Type &newValue) noexcept { storeRelease(newValue); return newValue; }

    // Atomic API, implemented in qatomic_XXX.h
    Type loadAcquire() const noexcept { return Ops::loadAcquire(m_x_value); }
    void storeRelease(const Type &newValue) noexcept { Ops::storeRelease(m_x_value, newValue); }

    static constexpr bool isTestAndSetNative() noexcept { return Ops::isTestAndSetNative(); }
    static constexpr bool isTestAndSetWaitFree() noexcept { return Ops::isTestAndSetWaitFree(); }

    bool testAndSetRelaxed(const Type &expectedValue,const Type &newValue) noexcept
    { return Ops::testAndSetRelaxed(m_x_value, expectedValue, newValue); }
    bool testAndSetAcquire(const Type &expectedValue,const Type &newValue) noexcept
    { return Ops::testAndSetAcquire(m_x_value, expectedValue, newValue); }
    bool testAndSetRelease(const Type &expectedValue,const Type &newValue) noexcept
    { return Ops::testAndSetRelease(m_x_value, expectedValue, newValue); }
    bool testAndSetOrdered(const Type &expectedValue,const Type &newValue) noexcept
    { return Ops::testAndSetOrdered(m_x_value, expectedValue, newValue); }

    bool testAndSetRelaxed(const Type &expectedValue,const Type &newValue, Type &currentValue) noexcept
    { return Ops::testAndSetRelaxed(m_x_value, expectedValue, newValue, &currentValue); }
    bool testAndSetAcquire(const Type &expectedValue,const Type &newValue, Type &currentValue) noexcept
    { return Ops::testAndSetAcquire(m_x_value, expectedValue, newValue, &currentValue); }
    bool testAndSetRelease(const Type &expectedValue,const Type &newValue, Type &currentValue) noexcept
    { return Ops::testAndSetRelease(m_x_value, expectedValue, newValue, &currentValue); }
    bool testAndSetOrdered(const Type &expectedValue,const Type &newValue, Type &currentValue) noexcept
    { return Ops::testAndSetOrdered(m_x_value, expectedValue, newValue, &currentValue); }

    static constexpr bool isFetchAndStoreNative() noexcept { return Ops::isFetchAndStoreNative(); }
    static constexpr bool isFetchAndStoreWaitFree() noexcept { return Ops::isFetchAndStoreWaitFree(); }

    Type fetchAndStoreRelaxed(const Type &newValue) noexcept
    { return Ops::fetchAndStoreRelaxed(m_x_value, newValue); }
    Type fetchAndStoreAcquire(const Type &newValue) noexcept
    { return Ops::fetchAndStoreAcquire(m_x_value, newValue); }
    Type fetchAndStoreRelease(const Type &newValue) noexcept
    { return Ops::fetchAndStoreRelease(m_x_value, newValue); }
    Type fetchAndStoreOrdered(const Type &newValue) noexcept
    { return Ops::fetchAndStoreOrdered(m_x_value, newValue); }

    static constexpr bool isFetchAndAddNative() noexcept { return Ops::isFetchAndAddNative(); }
    static constexpr bool isFetchAndAddWaitFree() noexcept { return Ops::isFetchAndAddWaitFree(); }

    Type fetchAndAddRelaxed(const xptrdiff &valueToAdd) noexcept
    { return Ops::fetchAndAddRelaxed(m_x_value, valueToAdd); }
    Type fetchAndAddAcquire(const xptrdiff &valueToAdd) noexcept
    { return Ops::fetchAndAddAcquire(m_x_value, valueToAdd); }
    Type fetchAndAddRelease(const xptrdiff &valueToAdd) noexcept
    { return Ops::fetchAndAddRelease(m_x_value, valueToAdd); }
    Type fetchAndAddOrdered(const xptrdiff &valueToAdd) noexcept
    { return Ops::fetchAndAddOrdered(m_x_value, valueToAdd); }

    Type fetchAndSubRelaxed(const xptrdiff &valueToAdd) noexcept
    { return Ops::fetchAndSubRelaxed(m_x_value, valueToAdd); }
    Type fetchAndSubAcquire(const xptrdiff &valueToAdd) noexcept
    { return Ops::fetchAndSubAcquire(m_x_value, valueToAdd); }
    Type fetchAndSubRelease(const xptrdiff &valueToAdd) noexcept
    { return Ops::fetchAndSubRelease(m_x_value, valueToAdd); }
    Type fetchAndSubOrdered(const xptrdiff &valueToAdd) noexcept
    { return Ops::fetchAndSubOrdered(m_x_value, valueToAdd); }

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

    XBasicAtomicPointer() = default;
    explicit constexpr XBasicAtomicPointer(const Type &value) noexcept : m_x_value(value) {}
    XBasicAtomicPointer(const XBasicAtomicPointer &) = delete;
    XBasicAtomicPointer &operator=(const XBasicAtomicPointer &) = delete;
    XBasicAtomicPointer &operator=(const XBasicAtomicPointer &) volatile = delete;
};

#ifndef X_BASIC_ATOMIC_INITIALIZER
#  define X_BASIC_ATOMIC_INITIALIZER(a) { (a) }
#endif

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
