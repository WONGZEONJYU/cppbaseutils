#include "xobject_p_p.hpp"
#include <iostream>
#include <tuple>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

XObject::XObject():m_d_ptr_(std::make_unique<XObjectPrivate>()) {}

XObject::~XObject() {
    X_D(XObject)
    if (const auto x{d->m_sharedRefcount_.loadRelaxed()}){
        if (x->m_strong_ref.loadRelaxed() > 0){
            std::cerr << "XObject: shared XObject was deleted directly. The program is malformed and may crash.";
        }

        x->m_strong_ref.storeRelaxed(0);
        if (!x->m_weak_ref.deref()){
            delete x;
        }
    }
}

bool XObject::connectImpl(const XObject * const sender, void ** const signal,
                     const XObject * const receiver, void ** const slot,
                     XPrivate::XSignalSlotBase * const slotObjRaw,
                     ConnectionType const type) {
    XPrivate::SlotObjUniquePtr slotObj{slotObjRaw};
    if (!signal){
        X_ASSERT_W(signal,"","XObject::connect: invalid nullptr parameter");
        return false;
    }
    return XObjectPrivate::connectImpl(sender,signal, receiver,slot,slotObj.release(),type);
}

void XObjectPrivate::ensureConnectionData() {
    if (m_connections.loadRelaxed()){
        return;
    }
    const auto cd{std::make_unique<XConnectionData>().release()};
    cd->m_ref.ref();
    m_connections.storeRelaxed(cd);
}

void XObjectPrivate::addConnection(XConnection *c){

}

void XObjectPrivate::removeConnection(XConnection * c)
{

}

bool XObjectPrivate::connectImpl(const XObject* sender, void** const signal,
                                 const XObject* const receiver, void** const slot,
                                 XPrivate::XSignalSlotBase* const slotObjRaw, ConnectionType const type) {

    XPrivate::SlotObjUniquePtr slotObj{slotObjRaw};

    if (!sender || !receiver || !slotObj ) {
        X_ASSERT_W(sender && receiver,"","invalid nullptr parameter");
        return {};
    }

    const auto [s,r]{
        std::tuple {const_cast<XObject*>(sender),
            const_cast<XObject *>(receiver)}
    };

    if (ConnectionType::UniqueConnection == type) {
        if (const auto connections{get(s)->m_connections.loadRelaxed()}) {
            if (const auto it {connections->m_connectionStorage.find(signal)};
                it != connections->m_connectionStorage.end()) {
                if (signal == it->first && it->second->slotRaw()->compare(signal)) {
                    return {};
                }
            }
        }
    }

    auto c {std::make_unique<XConnection>(slotObjRaw)};
    c->m_sender_ = s;
    c->m_receiver_ = r;
    get(s)->addConnection(c.release());
    return true;
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
