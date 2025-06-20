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
    mutable std::shared_ptr<XAbstractTask2Private> m_d_{};
    X_DECLARE_PRIVATE_D(m_d_,XAbstractTask2Private)
    std::any result_() const;

public:
    X_DEFAULT_COPY_MOVE(XAbstractTask2)
    virtual ~XAbstractTask2() = default;

    /// 用于获取线程执行完毕的返回值,如果没有可忽略
    /// 没有加入线程池或多次调用无效,不会阻塞但有警告提示
    /// 如果有返回值,T类型错误,本函数会抛出异常
    /// @tparam T
    /// @return T类型
    template<typename T>
    [[maybe_unused]] [[nodiscard]] T result() const noexcept(false) {
        const auto v{result_()};
        return v.has_value() ? std::any_cast<T>(v) : T{};
    }

    /// 设置责任链,开发者可以重写
    /// @param next_
    [[maybe_unused]] virtual void set_nextHandler(const std::weak_ptr<XAbstractTask2> &next_);

    /// 责任链请求处理
    /// @param arg 任意类型,开发者可重写
    [[maybe_unused]] virtual void requestHandler(const std::any &arg);

    /// 把自身加入线程池,会按照默认线程数量启动线程池,
    /// 如果需要调整数量(需在FIXED模式才有意义),请自行调用线程池start函数输入线程数量
    /// 如果在自身线程下调用再把自己加入,会出现无限加入的情况,请勿在这样操作
    /// 目前考虑是否禁止在自身线程下调用本函数(本备注随时删除)
    /// @param pool
    /// @return 任务对象
    [[maybe_unused]] XAbstractTask2_Ptr joinThreadPool(const std::shared_ptr<XThreadPool2> &pool) ;

protected:

    XAbstractTask2();

    /// 检查线程池是否运行
    /// @return  ture or false
    [[maybe_unused]] [[nodiscard]] bool is_running_() const;

    ///响应责任链的请求,需开发者自行重写
    /// @param arg
    [[maybe_unused]] virtual void responseHandler(const std::any &arg) {(void )arg;}

    /// 开发者必须重写本函数,本函数为线程执行函数
    /// @return 任意类型
    virtual std::any run() = 0;
private:
    void operator()();
    void set_result_(const std::any &) const;
    void set_exit_function_(std::function<bool()> &&) const;
    void set_occupy_() const;
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
