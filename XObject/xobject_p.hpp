#ifndef X_OBJECT_P_HPP
#define X_OBJECT_P_HPP 1

#include <xobject.hpp>
#include <XTools/xpointer.hpp>
#include <XAtomic/xatomic.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XObject;

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
