#ifndef X_OBJECT_P_HPP
#define X_OBJECT_P_HPP 1

#include <XObject/xobject.hpp>
#include <XAtomic/xatomic.hpp>
#include <XTools/xpointer.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XObjectPrivate final : public XObjectData {
    friend class XObject;
public:
    class XConnection;
    class XConnectionData;
    class XSender;

    explicit XObjectPrivate() = default;

    ~XObjectPrivate() override = default;

    void ensureConnectionData();
    void addConnection(std::size_t ,const std::shared_ptr<XConnection> &);

    static XObjectPrivate *get(XObject *o){return o->d_func();}
    static const XObjectPrivate *get (const XObject *o){return o->d_func();}

    static bool connectImpl(const XObject *sender, std::size_t signal_index,
                     const XObject *receiver, void **slot,
                     XPrivate::XSignalSlotBase * slotObjRaw,
                     ConnectionType type);

    static bool disconnectImpl(const XObject *sender, std::size_t signal_index,
        const XObject *receiver, void **slot);

    XAtomicPointer<ExternalRefCountData> m_sharedRefcount_{};

    XAtomicPointer<XConnectionData> m_connections{};
};

XTD_INLINE_NAMESPACE_END

XTD_NAMESPACE_END

#endif
