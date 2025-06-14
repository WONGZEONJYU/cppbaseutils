#ifndef X_ABSTRACT_TASK2_HPP
#define X_ABSTRACT_TASK2_HPP

#include <any>
#include <XHelper/xhelper.hpp>
#include <memory>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XAbstractTask2 : public std::enable_shared_from_this<XAbstractTask2>{
    friend class XThreadPool2;
    class XAbstractTask2Private;
    std::shared_ptr<XAbstractTask2Private> m_d_{};
    std::weak_ptr<XAbstractTask2Private> m_next_{};
protected:
    XAbstractTask2();
public:
    virtual ~XAbstractTask2() = default;
    template<typename T>
    T result() const noexcept(false){
        const auto v{result_()};
        return v.has_value() ? std::any_cast<T>(v) : T{};
    }
protected:
    bool is_running_() const;;
private:
    void exec_();
    virtual std::any run() = 0;
    std::any result_() const;
    void set_result_(const std::any &) const;
    void set_exit_function_();
};

using XAbstractTask2_Ptr = std::shared_ptr<XAbstractTask2>;

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
