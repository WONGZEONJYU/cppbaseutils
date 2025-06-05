#ifndef X_OBJECT_P_HPP
#define X_OBJECT_P_HPP 1

#include <atomic>
#include <xobject.hpp>
#include <XAtomic/xatomic.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XObject;

class ExternalRefCountData{
    enum class Private{};
public:
    explicit ExternalRefCountData(Private) {}
    XAtomicInt m_ref_{};
    [[maybe_unused]] static ExternalRefCountData *getAndRef(const XObject *);
};

class XObjectPrivate final {

public:
    explicit XObjectPrivate() = default;
    ~XObjectPrivate() = default;
    static XObjectPrivate *get(XObject *o){return o->d_func();}
    static const XObjectPrivate *get (const XObject *o){return o->d_func();}
    XAtomicPointer<ExternalRefCountData> m_sharedRefcount_{};
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
