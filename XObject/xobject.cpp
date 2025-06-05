#include "xobject_p.hpp"

[[maybe_unused]] xtd::v1::ExternalRefCountData * xtd::v1::ExternalRefCountData::getAndRef(const XObject *obj){

    X_ASSERT(obj);
    const auto d{XObjectPrivate::get(const_cast<XObject*>(obj))};
    if (const auto that{d->m_sharedRefcount_.loadAcquire()}){
        that->m_ref_.ref();
        return that;
    }

    try{
        auto x{::new ExternalRefCountData(Private{})};
        decltype(x) ret{};

        x->m_ref_.ref();
        if (d->m_sharedRefcount_.testAndSetOrdered(nullptr,x,ret)){
            ret = x;
        }else{

        }
        return ret;
    }catch (const std::bad_alloc &){
        return {};
    }
}

xtd::XObject::XObject():m_d_(std::make_unique<XObjectPrivate>()) {

}

xtd::XObject::~XObject() {
    if (const auto x{m_d_->m_sharedRefcount_.loadRelaxed()};
    x && !x->m_ref_.deref()){
        m_d_->m_sharedRefcount_.storeRelease(nullptr);
        delete x;
    }
}
