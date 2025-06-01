#ifndef CLION_TEST_X_OBJECT_HPP
#define CLION_TEST_X_OBJECT_HPP 1

#include "../XHelper/xhelper.hpp"
#include <memory>
#include <atomic>

class XPrivateData;

class XObject {
    X_DISABLE_COPY_MOVE(XObject)
public:
    using type_ptr = XObject*;
protected:
    explicit XObject();
    virtual ~XObject();
private:
    type_ptr m_this_{};
    std::unique_ptr<XPrivateData> m_d_{};
};

#endif
