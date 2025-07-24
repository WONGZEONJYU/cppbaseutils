#ifndef X_OBJECT_P_P_HPP
#define X_OBJECT_P_P_HPP 1

#include <XObject/xobject_p.hpp>
#include <XHelper/xhelper.hpp>
#include <list>
#include <map>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

static std::mutex g_mutex[131]{};

class XObjectPrivate::XSender final {
public:
    explicit XSender() = default;
    ~XSender() = default;
};

class XObjectPrivate::XConnection final : public std::enable_shared_from_this<XConnection> {

    mutable XPrivate::XSignalSlotBase * m_slot_raw_{};
public:
    mutable XObject * m_sender_{},* m_receiver_{};
    mutable XSender * m_currentSenders_{};

    explicit XConnection(XPrivate::XSignalSlotBase * slot_base = {}):
    m_slot_raw_(slot_base){}

    ~XConnection() {
        XPrivate::SlotObjUniquePtr slotObj{m_slot_raw_};
    }

    void call(XObject * r,void **args) const{
        m_slot_raw_->call(r,args);
    }

    [[nodiscard]] XPrivate::XSignalSlotBase * &slotRaw() const noexcept {
        return m_slot_raw_;
    }

    friend class XObject;
    X_DISABLE_COPY_MOVE(XConnection)
};

using XConnection_ptr = std::shared_ptr<XObjectPrivate::XConnection>;

class XObjectPrivate::XConnectionData {
public:
    XAtomicInt m_ref{};
    std::multimap<void **,XConnection_ptr> m_connectionStorage;
    std::list<XObject * > m_senders{};
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
