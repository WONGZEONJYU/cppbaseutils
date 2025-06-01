#include "xobject.hpp"
#include "xobject_p.hpp"

XObject::XObject():m_this_{this},
m_d_(std::make_unique<XPrivateData>()) {

}

XObject::~XObject() {
    if (const auto x{m_d_->m_sharedCount_.load()};
    x && int(1) == x->m_ref_.fetch_sub(1)){
        m_d_->m_sharedCount_ = {};
        delete x;
    }
}
