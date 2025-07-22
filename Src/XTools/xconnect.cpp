#include "xconnect.hpp"

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

XConnect::~XConnect() {
    if (m_slot_){
        m_slot_->destroyIfLastRef();
    }
}

void XConnect::call(XObject* r, void** args) const {
    m_slot_->call(r,args);
}

XObject*& XConnect::sender() const{
    return m_sender_;
}

XObject*& XConnect::receiver() const {
    return m_receiver_;
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
