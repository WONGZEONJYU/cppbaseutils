#ifndef X_ABSTRACT_TASK2_HPP
#define X_ABSTRACT_TASK2_HPP

#include <any>
#include <future>
#include <XHelper/xhelper.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XAbstractTask2 {
    std::future<std::any> m_result_{};
protected:
    XAbstractTask2() = default;
public:
    X_DISABLE_COPY(XAbstractTask2)
    X_DEFAULT_MOVE(XAbstractTask2)
    virtual ~XAbstractTask2() = default;
    template <typename T>
    T result(){
        return std::any_cast<T>(m_result_.get());
    }
private:
    void exec();
    virtual std::any run() = 0;
    friend class XThreadPool2;
};

using XAbstractTask2_Ptr = std::shared_ptr<XAbstractTask2>;

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
