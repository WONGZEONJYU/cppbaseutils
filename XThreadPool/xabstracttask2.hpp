#ifndef X_ABSTRACT_TASK2_HPP
#define X_ABSTRACT_TASK2_HPP

#include <any>
#include <memory>
#include <functional>
#include <XHelper/xhelper.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XThreadPool2;
class XAbstractTask2;
using XAbstractTask2_Ptr = std::shared_ptr<XAbstractTask2>;

class XAbstractTask2 : public std::enable_shared_from_this<XAbstractTask2> {
    friend class XThreadPool2;
    class XAbstractTask2Private;
    std::shared_ptr<XAbstractTask2Private> m_d_{};
    X_DECLARE_PRIVATE_D(m_d_,XAbstractTask2Private)
    std::any result_() const;

public:
    X_DEFAULT_COPY_MOVE(XAbstractTask2)
    virtual ~XAbstractTask2() = default;

    template<typename T>
    [[maybe_unused]] T result() const noexcept(false){
        const auto v{result_()};
        return v.has_value() ? std::any_cast<T>(v) : T{};
    }

    [[maybe_unused]] virtual void set_nextHandler(const std::weak_ptr<XAbstractTask2> &);

    [[maybe_unused]] virtual void requestHandler(const std::any &);

    [[maybe_unused]] XAbstractTask2_Ptr joinThreadPool(const std::shared_ptr<XThreadPool2> &) ;

protected:
    XAbstractTask2();
    [[maybe_unused]] bool is_running_() const;
    [[maybe_unused]] virtual void responseHandler(const std::any &) {}
private:
    void operator()();
    void set_result_(const std::any &) const;
    void set_exit_function_(std::function<bool()> &&) const;
    virtual std::any run() = 0;
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
