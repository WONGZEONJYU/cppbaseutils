#ifndef QATOMIC_H
#define QATOMIC_H

#include <XAtomic/xbasicatomic.h>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XAtomicBool : public XBasicAtomic<bool>{
    using Base_ = XBasicAtomic;
public:
    constexpr explicit XAtomicBool(const bool & value = {}) : XBasicAtomic(value){}

    inline XAtomicBool(const XAtomicBool &other) noexcept{
        this->storeRelease(other.loadAcquire());
    }

    inline XAtomicBool &operator=(const XAtomicBool &other) noexcept{
        this->storeRelease(other.loadAcquire());
        return *this;
    }
};

// High-level atomic integer operations
template <typename T>
class XAtomicInteger : public XBasicAtomicInteger<T>{
    using Base_ = XBasicAtomicInteger<T>;
public:
    // Non-atomic API
    constexpr explicit XAtomicInteger(const T &value = {}) noexcept : XBasicAtomicInteger<T>(value) {}

    inline XAtomicInteger(const XAtomicInteger &other) noexcept{
        this->storeRelease(other.loadAcquire());
    }

    inline XAtomicInteger &operator=(const XAtomicInteger &other) noexcept{
        this->storeRelease(other.loadAcquire());
        return *this;
    }
#ifdef X_XDOC
    T loadRelaxed() const;
    T loadAcquire() const;
    void storeRelaxed(T newValue);
    void storeRelease(T newValue);

    operator T() const;
    QAtomicInteger &operator=(T);

    static constexpr bool isReferenceCountingNative();
    static constexpr bool isReferenceCountingWaitFree();

    bool ref();
    bool deref();

    static constexpr bool isTestAndSetNative();
    static constexpr bool isTestAndSetWaitFree();

    bool testAndSetRelaxed(T expectedValue, T newValue);
    bool testAndSetAcquire(T expectedValue, T newValue);
    bool testAndSetRelease(T expectedValue, T newValue);
    bool testAndSetOrdered(T expectedValue, T newValue);

    bool testAndSetRelaxed(T expectedValue, T newValue, T &currentValue);
    bool testAndSetAcquire(T expectedValue, T newValue, T &currentValue);
    bool testAndSetRelease(T expectedValue, T newValue, T &currentValue);
    bool testAndSetOrdered(T expectedValue, T newValue, T &currentValue);

    static constexpr bool isFetchAndStoreNative();
    static constexpr bool isFetchAndStoreWaitFree();

    T fetchAndStoreRelaxed(T newValue);
    T fetchAndStoreAcquire(T newValue);
    T fetchAndStoreRelease(T newValue);
    T fetchAndStoreOrdered(T newValue);

    static constexpr bool isFetchAndAddNative();
    static constexpr bool isFetchAndAddWaitFree();

    T fetchAndAddRelaxed(T valueToAdd);
    T fetchAndAddAcquire(T valueToAdd);
    T fetchAndAddRelease(T valueToAdd);
    T fetchAndAddOrdered(T valueToAdd);

    T fetchAndSubRelaxed(T valueToSub);
    T fetchAndSubAcquire(T valueToSub);
    T fetchAndSubRelease(T valueToSub);
    T fetchAndSubOrdered(T valueToSub);

    T fetchAndOrRelaxed(T valueToOr);
    T fetchAndOrAcquire(T valueToOr);
    T fetchAndOrRelease(T valueToOr);
    T fetchAndOrOrdered(T valueToOr);

    T fetchAndAndRelaxed(T valueToAnd);
    T fetchAndAndAcquire(T valueToAnd);
    T fetchAndAndRelease(T valueToAnd);
    T fetchAndAndOrdered(T valueToAnd);

    T fetchAndXorRelaxed(T valueToXor);
    T fetchAndXorAcquire(T valueToXor);
    T fetchAndXorRelease(T valueToXor);
    T fetchAndXorOrdered(T valueToXor);

    T operator++();
    T operator++(int);
    T operator--();
    T operator--(int);
    T operator+=(T value);
    T operator-=(T value);
    T operator|=(T value);
    T operator&=(T value);
    T operator^=(T value);
#endif
};

class [[maybe_unused]] XAtomicInt : public XAtomicInteger<int>{
public:
    // Non-atomic API
    // We could use QT_COMPILER_INHERITING_CONSTRUCTORS, but we need only one;
    // the implicit definition for all the others is fine.
    constexpr explicit XAtomicInt(const int &value = {}) noexcept : XAtomicInteger(value) {}
};

// High-level atomic pointer operations
template <typename T>
class XAtomicPointer : public XBasicAtomicPointer<T>
{
public:

    constexpr explicit XAtomicPointer(T *value = nullptr) noexcept : XBasicAtomicPointer<T>(value) {}

    inline XAtomicPointer(const XAtomicPointer &other) noexcept
        : XBasicAtomicPointer<T>(){
        this->storeRelease(other.loadAcquire());
    }

    inline XAtomicPointer &operator=(const XAtomicPointer &other) noexcept{
        this->storeRelease(other.loadAcquire());
        return *this;
    }

#ifdef X_XDOC
    T *loadAcquire() const;
    T *loadRelaxed() const;
    void storeRelaxed(T *newValue);
    void storeRelease(T *newValue);

    static constexpr bool isTestAndSetNative();
    static constexpr bool isTestAndSetWaitFree();

    bool testAndSetRelaxed(T *expectedValue, T *newValue);
    bool testAndSetAcquire(T *expectedValue, T *newValue);
    bool testAndSetRelease(T *expectedValue, T *newValue);
    bool testAndSetOrdered(T *expectedValue, T *newValue);

    static constexpr bool isFetchAndStoreNative();
    static constexpr bool isFetchAndStoreWaitFree();

    T *fetchAndStoreRelaxed(T *newValue);
    T *fetchAndStoreAcquire(T *newValue);
    T *fetchAndStoreRelease(T *newValue);
    T *fetchAndStoreOrdered(T *newValue);

    static constexpr bool isFetchAndAddNative();
    static constexpr bool isFetchAndAddWaitFree();

    T *fetchAndAddRelaxed(qptrdiff valueToAdd);
    T *fetchAndAddAcquire(qptrdiff valueToAdd);
    T *fetchAndAddRelease(qptrdiff valueToAdd);
    T *fetchAndAddOrdered(qptrdiff valueToAdd);
#endif
};


/*!
    This is a helper for the assignment operators of implicitly
    shared classes. Your assignment operator should look like this:

*/
template <typename T>
inline void qAtomicAssign(T *&d,T *x){
    if (d == x){
        return;
    }
    x->ref.ref();
    if (!d->ref.deref()){
        delete d;
    }
    d = x;
}

/*!
    This is a helper for the detach method of implicitly shared
    classes. Your private class needs a copy constructor which copies
    the members and sets the refcount to 1. After that, your detach
    function should look like this:
*/

template <typename T>
inline void qAtomicDetach(T *&d){
    if (d->ref.loadRelaxed() == 1){
        return;
    }

    T *x {d};
    d = new T(*d);
    if (!x->ref.deref()){
        delete x;
    }
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
