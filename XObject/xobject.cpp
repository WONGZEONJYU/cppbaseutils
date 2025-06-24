#include "xobject_p.hpp"
#include <iostream>

xtd::XObject::XObject():m_d_ptr_(std::make_unique<XObjectPrivate>()) {

}

xtd::XObject::~XObject() {
    if (const auto x{m_d_ptr_->m_sharedRefcount_.loadRelaxed()}){
        if (x->m_strong_ref.loadRelaxed() > 0){
            std::cerr << "XObject: shared XObject was deleted directly. The program is malformed and may crash.";
        }
        x->m_strong_ref.storeRelaxed(0);
        if (!x->m_weak_ref.deref()){
            delete x;
        }
    }
}
