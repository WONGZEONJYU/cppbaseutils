#ifndef X_ABSTRACT_TASK2_HPP
#define X_ABSTRACT_TASK2_HPP

#include <any>
#include <future>
#include <XHelper/xhelper.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XAbstractTask2 {
    std::promise<std::any> m_promise_{};
protected:
    XAbstractTask2() = default;
public:
    virtual ~XAbstractTask2() = default;
    template<typename T>
    T result() noexcept(false){
        const auto v{m_promise_.get_future().get()};
        return v.has_value() ? std::any_cast<T>(v) : T{};
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
