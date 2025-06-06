#include "xpointer.hpp"
#include <XObject/xobject_p.hpp>

[[maybe_unused]] xtd::v1::ExternalRefCountData * xtd::v1::ExternalRefCountData::getAndRef(const XObject *obj){

    X_ASSERT(obj);
    const auto d{XObjectPrivate::get(const_cast<XObject*>(obj))};
    if (const auto that{d->m_sharedRefcount_.loadAcquire()}){
        that->m_weak_ref.ref();
        return that;
    }

    try{
        auto x{::new ExternalRefCountData(Private{})};
        x->m_strong_ref.storeRelaxed(-1);
        x->m_weak_ref.storeRelaxed(2);
        decltype(x) ret{};
        if (d->m_sharedRefcount_.testAndSetOrdered(nullptr,x,ret)){
            ret = x;
        }else{
            X_ASSERT((x->m_weak_ref.storeRelaxed(0),true));
            ::delete x;
            ret->m_weak_ref.ref();
        }
        return ret;
    }catch (const std::bad_alloc &){
        return {};
    }
}





