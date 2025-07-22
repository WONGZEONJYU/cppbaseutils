#ifndef X_CONNECT_HPP
#define X_CONNECT_HPP 1

#include <XHelper/xhelper.hpp>
#include <XTools/xsignalslot.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XObject;

class XConnect final : public std::enable_shared_from_this<XConnect> {
    mutable XPrivate::XSignalSlotBase * m_slot_{};
    mutable XObject * m_sender_{},* m_receiver_{};
public:
    explicit XConnect() = default;
    ~XConnect();
private:
    void call(XObject * r,void **args) const;
    [[maybe_unused]] XObject* &sender() const;
    XObject* &receiver() const;
    friend class XObject;
    X_DISABLE_COPY_MOVE(XConnect)
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
