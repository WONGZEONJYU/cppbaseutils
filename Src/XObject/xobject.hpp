#ifndef CLION_TEST_X_OBJECT_HPP
#define CLION_TEST_X_OBJECT_HPP 1

#include <XHelper/xhelper.hpp>
#include <memory>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XObjectPrivate;

class XObject : public  std::enable_shared_from_this<XObject> {
    X_DISABLE_COPY_MOVE(XObject)
    X_DECLARE_PRIVATE(XObject)
protected:
    explicit XObject();
    virtual ~XObject();
private:
    std::unique_ptr<XObjectPrivate> m_d_ptr_{};
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
