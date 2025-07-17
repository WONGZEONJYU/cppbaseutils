#include "xobject_p.hpp"
#include <iostream>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

XObject::XObject():m_d_ptr_(std::make_unique<XObjectPrivate>()) {

}

XObject::~XObject() {
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

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
