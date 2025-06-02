#ifndef X_OBJECT_P_HPP
#define X_OBJECT_P_HPP 1

#include <atomic>
#include "xobject.hpp"

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XObject;

class ExternalRefCountData{

public:
    explicit ExternalRefCountData() = default;
    std::atomic_int m_ref_{};
    static ExternalRefCountData * getAndRef(const XObject *);
};

class XObjectPrivate final {

public:
    explicit XObjectPrivate() = default;
    ~XObjectPrivate() = default;
    static XObjectPrivate *get(XObject *o){return o->d_func();}
    static const XObjectPrivate *get (const XObject *o){return o->d_func();}
    std::atomic<ExternalRefCountData*> m_sharedRefcount_{};
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
