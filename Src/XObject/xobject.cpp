#include "xobject_p_p.hpp"
#include <XThreadPool/xorderedmutexlocker_p.hpp>
#include <iostream>
#include <tuple>
#include <mutex>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

#if __cplusplus >= 202002L
constinit
#endif
inline static std::mutex x_Object_MutexPool[131]{};

[[maybe_unused]] inline static std::mutex *signalSlotLock(const XObject * const o) {
#if __cplusplus >= 201703L
    static constexpr auto num{std::size(x_Object_MutexPool)};
#else
    static constexpr auto num{sizeof(x_Object_MutexPool) / sizeof(x_Object_MutexPool[0])};
#endif
    const auto index{static_cast<std::size_t>(reinterpret_cast<xuintptr>(o)) % num};
    return std::addressof(x_Object_MutexPool[index]);
}

XObject::XObject():m_d_ptr_(std::make_unique<XObjectPrivate>()) {}

XObject::~XObject() {
    X_D(XObject)

    if (const auto cd{d->m_connections.loadRelaxed()};
        cd && !cd->m_ref.deref()){
        delete cd;
        d->m_connections.storeRelease({});
    }

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

bool XObject::blockSignals(bool b) noexcept {
    X_D(XObject)
    const auto previous{static_cast<bool>(d->m_blockSig)};
    d->m_blockSig = b;
    return previous;
}

XObject *XObject::sender() const {
    X_D(const XObject)

    std::unique_lock locker(*signalSlotLock(this));

    auto const cd{d->m_connections.loadRelaxed()};
    if (!cd || !cd->m_currentSender){
        return {};
    }

    auto const currentSender{cd->m_currentSender};
    auto const &senders{cd->m_senders};
    for (auto const & c:senders){
        if (c.expired() && c.lock()->m_sender == currentSender->m_sender){
            return currentSender->m_sender;
        }
    }

    return {};
}

std::size_t XObject::senderSignalIndex() const {
    X_D(const XObject)
    std::unique_lock locker(*signalSlotLock(this));
    auto const cd{d->m_connections.loadRelaxed()};
    if (!cd || !cd->m_currentSender){
        return {};
    }

    auto const currentSender{cd->m_currentSender};
    auto const &senders{cd->m_senders};
    for (auto const & c:senders) {
        if (c.expired() && c.lock()->m_sender == currentSender->m_sender){
            return c.lock()->m_signal_index;
        }
    }

    return {};
}

bool XObject::connectImpl(const XObject * const sender, void ** const signal,
                     const XObject * const receiver, void ** const slot,
                     XPrivate::XSignalSlotBase * const slotObjRaw,
                     ConnectionType const type) {
    XPrivate::SlotObjUniquePtr slotObj{slotObjRaw};

    if (!signal){
        X_ASSERT_W(signal,"","XObject::connect: invalid nullptr parameter");
        return {};
    }
    const auto signal_index{std::hash<void*>{}(*signal)};
    return XObjectPrivate::connectImpl(sender,signal_index,
        receiver,slot,slotObj.release(),type);
}

bool XObject::disconnectImpl(const XObject* const sender, void** const signal,
    const XObject* const receiver, void** const slot) {

    if (!sender || (!receiver && slot)) {
        X_ASSERT_W(false,"","XObject::disconnect: invalid nullptr parameter");
        return {};
    }

    std::size_t signal_index{};
    if (signal){
        signal_index = std::hash<void*>{}(*signal);
    }

    return XObjectPrivate::disconnectImpl(sender,signal_index,receiver,slot);
}

void XObjectPrivate::ensureConnectionData() {
    if (m_connections.loadRelaxed()){
        return;
    }
    const auto cd{std::make_unique<XConnectionData>().release()};
    cd->m_ref.ref();
    m_connections.storeRelaxed(cd);
}

void XObjectPrivate::addConnection(std::size_t const signal_index,
const std::shared_ptr<XConnection> &c){

    ensureConnectionData();
    const auto cd{m_connections.loadRelaxed()};
    cd->resizeSignalVector(signal_index);

    auto &connectList {cd->connectionsForSignal(signal_index)};
    connectList.push_back(c);

    const auto rd{get(c->m_receiver.loadRelaxed())};
    rd->ensureConnectionData();
    rd->m_connections.loadAcquire()->m_senders.push_front(c);
}

bool XObjectPrivate::connectImpl(const XObject* sender, std::size_t const signal_index,
                                 const XObject* const receiver, void** const slot,
                                 XPrivate::XSignalSlotBase* const slotObjRaw, ConnectionType const type) {

    XPrivate::SlotObjUniquePtr slotObj{slotObjRaw};

    if (!sender || !receiver || !slotObj ) {
        X_ASSERT_W(false,"","invalid nullptr parameter");
        return {};
    }

    if (type == ConnectionType::UniqueConnection && !slot){
        X_ASSERT_W(false,"","unique connections require a pointer to member function of a XObject subclass");
        return {};
    }

    const auto [s,r]{
        std::tuple {const_cast<XObject*>(sender),
            const_cast<XObject *>(receiver)}
    };

    XOrderedMutexLocker locker(signalSlotLock(sender),signalSlotLock(receiver));

    if (slot && ConnectionType::UniqueConnection == type) {
        if (const auto connections{get(s)->m_connections.loadRelaxed()}) {
            if (const auto &sigVector{connections->m_signalVector};
                !sigVector.empty()) {
                if (auto const clist{sigVector.find(signal_index)}; clist != sigVector.end()){
                    for (auto const & item:clist->second){
                        if (receiver == item->m_receiver.loadRelaxed()
                            && item->m_isSlotObject
                            && item->m_slot_raw->compare(slot)){
                            return {};
                        }
                    }
                }
            }
        }
    }

    const auto c {std::make_shared<XConnection>(slotObj.release())};
    c->m_sender = s;
    c->m_receiver.storeRelease(r);
    c->m_signal_index = signal_index;
    c->m_isSlotObject = true;
    get(s)->addConnection(signal_index,c);
    return true;
}

bool XObjectPrivate::disconnectImpl(const XObject* const sender,std::size_t const signal_index,
    const XObject* const receiver, void** const slot) {

    if (!sender) {
        return {};
    }

    const auto s{const_cast<XObject*>(sender)};

    auto cd{get(s)->m_connections.loadAcquire()};
    if (!cd){
        return {};
    }

    XOrderedMutexLocker locker(signalSlotLock(sender),signalSlotLock(receiver));

    auto &SignalVector{cd->m_signalVector};

    if (signal_index && receiver && slot) {

        auto const connectLists_it {SignalVector.find(signal_index)};
        if (SignalVector.end() == connectLists_it) {
            return {};
        }

        auto &connectLists{connectLists_it->second};

        for (auto c {connectLists.begin()}; c != connectLists.end();) {
            if (receiver == c->get()->m_receiver.loadRelaxed()
                && c->get()->m_slot_raw->compare(slot)
                && c->get()->m_isSlotObject) {
                c = connectLists.erase(c);
            }else {
                ++c;
            }
        }

        if (connectLists.empty()){
            SignalVector.erase(signal_index);
        }

    }else if (!signal_index && receiver && !slot) {

        for (auto vecIt {SignalVector.begin()}; vecIt != SignalVector.end();){

            auto &connectLists{vecIt->second};

            for (auto c {connectLists.begin()}; c != connectLists.end();){

                if (receiver == c->get()->m_receiver.loadRelaxed()
                    && c->get()->m_isSlotObject){
                    c = connectLists.erase(c);
                }else{
                    ++c;
                }
            }

            if (connectLists.empty()){
                vecIt = SignalVector.erase(vecIt);
            }else{
                ++vecIt;
            }
        }
    }else if (signal_index && !receiver && !slot) {
        if (SignalVector.contains(signal_index)){
            SignalVector.erase(signal_index);
        }
    }else {
        SignalVector.clear();
    }

    return true;
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
