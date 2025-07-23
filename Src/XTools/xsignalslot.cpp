#include "xsignalslot.hpp"

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace XPrivate {

XSignalSlotBase::XSignalSlotBase(ImplFn const fn) noexcept :m_impl_(fn) {}

bool XSignalSlotBase::ref() noexcept{
    return m_ref_.ref();
}

#if XTD_VERSION < XTD_VERSION_CHECK(0,0,2)
void XSignalSlotBase::destroyIfLastRef() noexcept {
    if (!m_ref_.deref()){
        m_impl_(Destroy, this, nullptr, nullptr, nullptr);
    }
}

bool XSignalSlotBase::compare(void ** const a) {
    bool ret{};
    m_impl_(Compare, this, nullptr, a, &ret);
    return ret;
}

void XSignalSlotBase::call(XObject * const r, void ** const a) {
    m_impl_(Call, this, r, a, nullptr);
}
#else
void XSignalSlotBase::destroyIfLastRef() noexcept {
    if (!m_ref_.deref()) {
        m_impl_(this, nullptr, nullptr, Destroy, nullptr);
    }
}

bool XSignalSlotBase::compare(void ** const a) {
    bool ret {};
    m_impl_(this, nullptr, a, Compare, &ret);
    return ret;
}

void XSignalSlotBase::call(XObject * const r, void ** const a) {
    m_impl_(this, r, a, Call, nullptr);
}
#endif

        [[maybe_unused]] SlotObjSharedPtr:: SlotObjSharedPtr(std::nullptr_t) noexcept
:SlotObjSharedPtr(){}

        [[maybe_unused]] SlotObjSharedPtr::SlotObjSharedPtr(SlotObjUniquePtr o)
:m_obj_(std::move(o)){}

SlotObjSharedPtr::SlotObjSharedPtr(const SlotObjSharedPtr &other) noexcept
:m_obj_{copy(other.m_obj_)} {}

SlotObjSharedPtr& SlotObjSharedPtr::operator=(const SlotObjSharedPtr &other) noexcept {
    auto copy{other};
    swap(copy);
    return *this;
}

void SlotObjSharedPtr::swap(SlotObjSharedPtr &other) noexcept {
    m_obj_.swap(other.m_obj_);
}

}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
