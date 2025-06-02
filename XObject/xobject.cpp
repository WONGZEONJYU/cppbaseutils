#include "xobject_p.hpp"

xtd::v1::ExternalRefCountData * xtd::v1::ExternalRefCountData::getAndRef(const XObject *obj){

    X_ASSERT(obj);
    const auto d{XObjectPrivate::get(const_cast<XObject*>(obj))};
    if (const auto that{d->m_sharedRefcount_.load(std::memory_order_acquire)}){
        that->m_ref_.fetch_add(1);
        return that;
    }

    try{
        auto x{::new ExternalRefCountData()};
        decltype(x) ret{};
        x->m_ref_.fetch_add(1);
        if (d->m_sharedRefcount_.compare_exchange_strong(ret,x)){
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
    if (const auto x{m_d_->m_sharedRefcount_.load()};
    x && 1 == x->m_ref_.fetch_sub(1)){
        m_d_->m_sharedRefcount_ = {};
        delete x;
    }
}
