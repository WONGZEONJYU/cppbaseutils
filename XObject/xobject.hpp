#ifndef CLION_TEST_X_OBJECT_HPP
#define CLION_TEST_X_OBJECT_HPP 1

#include "../XHelper/xhelper.hpp"
#include <memory>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XObjectPrivate;

class XObject {

    X_DISABLE_COPY_MOVE(XObject)
    X_DECLARE_PRIVATE_D(m_d_,XObjectPrivate)

protected:
    explicit XObject();
    virtual ~XObject();
private:
    std::unique_ptr<XObjectPrivate> m_d_{};
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
