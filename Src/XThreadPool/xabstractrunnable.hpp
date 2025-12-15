#ifndef XUTILS2_X_ABSTRACT_RUNNABLE_HPP
#define XUTILS2_X_ABSTRACT_RUNNABLE_HPP 1

#include <XHelper/xhelper.hpp>
#include <any>
#include <memory>
#include <functional>
#include <XAtomic/xatomic.hpp>
#include <XThreadPool/xresult.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XAbstractRunnablePrivate;
class XThreadPool;
class XAbstractRunnable;
using XAbstractRunnablePtr = std::shared_ptr<XAbstractRunnable>;

class X_CLASS_EXPORT XAbstractRunnableData {
    X_DISABLE_COPY_MOVE(XAbstractRunnableData)
    friend class XAbstractRunnable;
    XResult m_result_{};

public:
    XAbstractRunnable * m_x_ptr{};
protected:
    constexpr XAbstractRunnableData() = default;
public:
    virtual ~XAbstractRunnableData() = default;
};

class X_CLASS_EXPORT XAbstractRunnable : public std::enable_shared_from_this<XAbstractRunnable> {

    enum class FuncVer {CONST,NON_CONST};

    X_DECLARE_PRIVATE(XAbstractRunnable)
    mutable std::shared_ptr<XAbstractRunnableData> m_d_ptr_{};

public:
    X_DEFAULT_COPY_MOVE(XAbstractRunnable)

    enum class Model {BLOCK,NONBLOCK};

    /// 用于获取线程执行完毕的返回值,如果没有可忽略
    /// 没有加入线程池或多次调用无效,不会阻塞但有警告提示
    /// 如果有返回值,T类型错误,本函数会抛出异常
    /// 选择非阻塞模式,无论有无值,立即返回
    /// @param model_
    /// @tparam Ty
    /// @return T类型
    template<typename Ty>
    [[maybe_unused]] [[nodiscard]] constexpr Ty result(const Model& model_ = Model::BLOCK) const noexcept(false) {
        const auto & r{m_d_ptr_->m_result_};
        return Model::BLOCK == model_ ? r.get<Ty>() : r.try_get<Ty>();
    }

    /// 带超时等待返回值
    /// @tparam Ty
    /// @tparam Rep_
    /// @tparam Period_
    /// @param rel_time
    /// @return Ty类型数据
    template<typename Ty,typename Rep_,typename Period_>
    [[maybe_unused]] [[nodiscard]] constexpr Ty result(const std::chrono::duration<Rep_,Period_> &rel_time) const noexcept(false)
    { return m_d_ptr_->m_result_.get_for<Ty>(rel_time); }

    /// 带指定时间等候返回值
    /// @tparam Ty
    /// @tparam Clock_
    /// @param abs_time_
    /// @return Ty类型
    template<typename Ty,typename Clock_,typename Duration_>
    [[maybe_unused]] [[nodiscard]] constexpr Ty result(const std::chrono::time_point<Clock_,Duration_> & abs_time_) const noexcept(false)
    { return m_d_ptr_->m_result_.get_until<Ty>(abs_time_); }

    /// 加入线程池,会按照默认线程数量启动线程池
    /// 如果需要调整数量(需在FIXED模式才有意义),请自行调用线程池start函数输入线程数量
    /// @param pool
    /// @return 任务对象
    [[maybe_unused]] XAbstractRunnablePtr joinThreadPool(std::shared_ptr<XThreadPool> const & pool) noexcept;

    /// 检查线程池是否运行
    /// @return  ture or false
    [[maybe_unused]] [[maybe_unused]] [[nodiscard]] bool is_Running() const noexcept;

    virtual ~XAbstractRunnable() = default;

protected:
    ///给开发者制作私有构造函数
    enum class PrivateConstruct{};
    ///响应责任链的请求,需开发者自行重写
    /// @param
    [[maybe_unused]] constexpr virtual void responseHandler(std::any const & ) {}

private:
    virtual std::any run();
    virtual std::any run() const;
    explicit XAbstractRunnable(FuncVer );
    void call() const;
    void set_exit_function_(std::function<bool()> &&) const noexcept;
    void resetRecall_() const noexcept;
    void allow_get_() const noexcept;
    XAtomicPointer<const void> &Owner_() const noexcept;

    template<typename> friend class XRunnable;
    friend class XThreadPool;
    friend class XThreadPoolPrivate;
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
