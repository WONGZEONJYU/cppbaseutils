#ifndef XPOINTER_HPP
#define XPOINTER_HPP

#include <type_traits>
#include <cstddef>
#include <XAtomic/xatomic.hpp>
#include <XHelper/xhelper.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XObject;

class X_CLASS_EXPORT ExternalRefCountData {
protected:
    enum class Private{};
public:
    using Type = ExternalRefCountData;
    explicit ExternalRefCountData(Private) {}
    XAtomicInt m_weak_ref{}, m_strong_ref{};
    ~ExternalRefCountData() {
        X_ASSERT(!m_weak_ref.loadRelaxed());
        X_ASSERT(m_strong_ref.loadRelaxed() <= 0);
    }

    [[maybe_unused]] static Type *getAndRef(const XObject *);
};

template<typename T>
class X_TEMPLATE_EXPORT [[maybe_unused]] XPointer final {
    using Type = T;
    using Ptype = Type*;
    using RefType = Type&;
    using Data = ExternalRefCountData;
    static_assert(!std::is_pointer_v<T>, "XPointer's template type must not be a pointer type");
public:
    XPointer() = default;
    explicit constexpr XPointer(std::nullptr_t){}
    explicit XPointer(T* ptr):m_ptr_(ptr),m_d_(ptr ? Data::getAndRef(ptr) : nullptr) {}

    XPointer(const XPointer& rhs):m_ptr_(rhs.m_ptr_),m_d_(rhs.m_d_){
        if (m_d_){
            m_d_->m_weak_ref.ref();
        }
    }

    XPointer(XPointer&& rhs) noexcept:m_ptr_(rhs.m_ptr_),m_d_(rhs.m_d_) {
        rhs.m_ptr_ = nullptr;
        rhs.m_d_ = nullptr;
    }

    XPointer& operator=(const XPointer& rhs){
        XPointer(rhs).swap(*this);
        return *this;
    }

    XPointer& operator=(XPointer&& rhs) noexcept{
        XPointer(std::move(rhs)).swap(*this);
        return *this;
    }

    XPointer& operator=(T *ptr) noexcept{
        if (ptr){
            XPointer(ptr).swap(*this);
        }
        return *this;
    }

    void assign(T* ptr){
        if (!ptr){return;}
        XPointer(ptr).swap(*this);
    }

    void swap(XPointer& rhs) noexcept{
        std::swap(m_ptr_, rhs.m_ptr_);
        std::swap(m_d_, rhs.m_d_);
    }

    [[nodiscard]] bool isNull() const{
        return m_d_ == nullptr || m_ptr_ == nullptr || !m_d_->m_strong_ref.loadRelaxed();
    }

    [[maybe_unused]] [[nodiscard]] bool is_empty() const{
        return isNull();
    }

    explicit operator bool() const{
        return !isNull();
    }

    bool operator!() const{
        return isNull();
    }

    Ptype get() const{
        return !m_d_ || !m_d_->m_strong_ref.loadRelaxed() ? nullptr : m_ptr_;
    }

    Ptype operator->() const{
        return get();
    }

    explicit operator Ptype() const{
        return get();
    }

    RefType operator*() const{
        return *get();
    }

    ~XPointer(){
        if (m_d_ && !m_d_->m_weak_ref.deref()){
            delete m_d_;
        }
    }
private:
    Type* m_ptr_{};
    Data *m_d_{};
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
