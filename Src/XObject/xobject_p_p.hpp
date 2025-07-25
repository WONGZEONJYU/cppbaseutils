#ifndef X_OBJECT_P_P_HPP
#define X_OBJECT_P_P_HPP 1

#include <XObject/xobject_p.hpp>
#include <XHelper/xhelper.hpp>
#include <list>
#include <unordered_map>
#include <iostream>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XObjectPrivate::XSender final {
public:
    explicit XSender() = default;
    ~XSender() = default;
};

class XObjectPrivate::XConnection final : public std::enable_shared_from_this<XConnection> {
    friend class XObject;
    X_DISABLE_COPY_MOVE(XConnection)
public:
    XObject * m_sender{};
    XAtomicPointer<XObject> m_receiver{};
    std::size_t m_signal_index{};
    XPrivate::XSignalSlotBase * m_slot_raw{};
    uint m_isSlotObject:1;

    explicit XConnection(XPrivate::XSignalSlotBase * const slot_base = {}):
    m_slot_raw(slot_base),m_isSlotObject{}{}

    ~XConnection() {
        std::cerr << FUNC_SIGNATURE << "\n";
        XPrivate::SlotObjUniquePtr slotObj{m_slot_raw};
    }
};

using XConnection_SPtr = std::shared_ptr<XObjectPrivate::XConnection>;
using XConnection_WPtr = std::weak_ptr<XObjectPrivate::XConnection>;

class XObjectPrivate::XConnectionData final {
    using XConnectionList = std::list<XConnection_SPtr>;
public:
    explicit XConnectionData() = default;
    ~XConnectionData() = default;

    XAtomicInt m_ref{};
    std::unordered_map<std::size_t,XConnectionList> m_signalVector;
    std::list<XConnection_WPtr> m_senders{};
    XSender * m_currentSenders{};

    inline void resizeSignalVector(std::size_t const signal_index){
        if (!m_signalVector.contains(signal_index)){
            m_signalVector.reserve(1);
        }
    }

    inline XConnectionList& connectionsForSignal(size_t const signal_index) {
        return m_signalVector[signal_index];
    }
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
