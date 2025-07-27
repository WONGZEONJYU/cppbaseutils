#ifndef X_OBJECT_P_P_HPP
#define X_OBJECT_P_P_HPP 1

#include <XObject/xobject_p.hpp>
#include <XHelper/xhelper.hpp>
#include <list>
#include <unordered_map>
#include <iostream>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XObjectPrivate::XConnection final
        : public std::enable_shared_from_this<XConnection> {
    friend class XObject;
    X_DISABLE_COPY_MOVE(XConnection)
public:
    [[maybe_unused]] XObject * m_sender{};
    XAtomicPointer<XObject> m_receiver{};
    [[maybe_unused]] std::size_t m_signal_index{};
    XPrivate::XSignalSlotBase * m_slot_raw{};
    uint m_isSlotObject:1;

    explicit XConnection(XPrivate::XSignalSlotBase * const slot_base = {})
    :m_slot_raw(slot_base),m_isSlotObject{}
    {}

    ~XConnection() {
        std::cerr << FUNC_SIGNATURE << "\n";
        XPrivate::SlotObjUniquePtr slotObj{m_slot_raw};
    }
};

using XConnection_SPtr = std::shared_ptr<XObjectPrivate::XConnection>;
using XConnection_WPtr = std::weak_ptr<XObjectPrivate::XConnection>;
using XConnectionList = std::list<XConnection_SPtr>;
using XSendersList = std::list<XConnection_WPtr>;

class XObjectPrivate::XSignalVector final
        : public std::unordered_map<std::size_t ,XConnectionList> {
public:
    explicit XSignalVector() = default;
    ~XSignalVector() = default;
};

class XObjectPrivate::XConnectionData final {
public:
    explicit XConnectionData() = default;
    ~XConnectionData() {
        if(auto v{m_signalVector.loadRelaxed()}) {
            delete v;
            m_signalVector.storeRelaxed({});
        }
    }

    XAtomicInt m_ref{};
    XAtomicPointer<XSignalVector> m_signalVector{};
    XSendersList m_senders{};
    XSender * m_currentSender{};

    inline void resizeSignalVector(std::size_t const signal_index) {

        auto v{m_signalVector.loadRelaxed()};
        if(!v){
            v = make_Unique<XSignalVector>().release();
            m_signalVector.storeRelaxed(v);
        }
        v->try_emplace(signal_index);
    }

    inline XConnectionList& connectionsForSignal(size_t const signal_index) {
        return m_signalVector.loadRelaxed()->at(signal_index);
    }
};

class XObjectPrivate::XSender final {
public:
    [[maybe_unused]] XSender(XObject * const receiver, XObject * const sender, std::size_t const signal, XConnectionData * const receiverConnections)
            : m_receiver(receiver), m_sender(sender), m_signal(signal)
    {
        if (receiverConnections) {
            m_previous = receiverConnections->m_currentSender;
            receiverConnections->m_currentSender = this;
        }
    }

    ~XSender() {
        if (m_receiver){
            m_receiver->d_func()->m_connections.loadAcquire()->m_currentSender = m_previous;
        }
    }

    [[maybe_unused]] void receiverDeleted() {
        XSender *s = this;
        while (s) {
            s->m_previous = {};
            s = s->m_previous;
        }
    }

    XSender *m_previous{};
    [[maybe_unused]] XObject *m_receiver{},*m_sender{};
    [[maybe_unused]] std::size_t m_signal{};
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
