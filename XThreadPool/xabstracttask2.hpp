#ifndef X_ABSTRACT_TASK2_HPP
#define X_ABSTRACT_TASK2_HPP

#include <XHelper/xhelper.hpp>
#include <any>
#include <memory>
#include <functional>
#include <iostream>
#if _LIBCPP_STD_VER >= 20
#include <semaphore>
#else
#include <XThreadPool/xsemaphore.hpp>
#endif

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

public:
    X_DEFAULT_COPY_MOVE(XAbstractTask2)
    virtual ~XAbstractTask2() = default;

    enum class Model{
        BLOCK,///@brief 阻塞
        NONBLOCK,///@brief 非阻塞
    };

    static constexpr auto BlockModel{Model::BLOCK},NonblockModel{Model::NONBLOCK};
    /// 用于获取线程执行完毕的返回值,如果没有可忽略
    /// 没有加入线程池或多次调用无效,不会阻塞但有警告提示
    /// 如果有返回值,T类型错误,本函数会抛出异常
    /// 选择非阻塞模式,无论有无值,立即返回
    /// @param model_
    /// @tparam Ty
    /// @return T类型
    template<typename Ty>
    [[maybe_unused]] [[nodiscard]] Ty result(const Model& model_ = BlockModel) const noexcept(false) {
        const auto v{result_(model_)};
        return v.has_value() ? std::any_cast<Ty>(v) : Ty{};
    }

    /// 带超时等待返回值
    /// @tparam Ty
    /// @tparam Rep_
    /// @tparam Period_
    /// @param rel_time
    /// @return Ty类型数据
    template<typename Ty,typename Rep_,typename Period_>
    [[maybe_unused]] [[nodiscard]] Ty result(const std::chrono::duration<Rep_,Period_> &rel_time){
        const auto v{result_for_(std::chrono::duration_cast<std::chrono::nanoseconds>(rel_time))};
        return v.has_value() ? std::any_cast<Ty>(v) : Ty{};
    }

    /// 带指定时间等候返回值
    /// @tparam Ty
    /// @tparam Clock_
    /// @param abs_time_
    /// @return Ty类型
    template<typename Ty,typename Clock_,typename Duration_>
    [[maybe_unused]] [[nodiscard]] Ty result(const std::chrono::time_point<Clock_,Duration_> & abs_time_){

        constexpr std::string_view selfname{__PRETTY_FUNCTION__};
#if 1
        const XRAII raii{[&selfname]{
            std::cout << selfname << " begin\n" << std::flush;
        },[&selfname]{
            std::cout << selfname << " end\n" << std::flush;
        }};
#endif
        if (!is_running_()){
            std::cerr << selfname << " tips: tasks not added\n" << std::flush;
            return {};
        }

        if (!(*this)(nullptr).try_acquire_until(abs_time_)){
            return Ty{};
        }

        const auto v{Return_()};
        return v.has_value() ? std::any_cast<Ty>(v) : Ty{};
    }

    /// 设置责任链,开发者可以重写
    /// @param next_
    [[maybe_unused]] virtual void set_nextHandler(const std::weak_ptr<XAbstractTask2> &next_);

    /// 责任链请求处理
    /// @param arg 任意类型,开发者可重写
    [[maybe_unused]] virtual void requestHandler(const std::any &arg);

    /// 加入线程池,会按照默认线程数量启动线程池
    /// 如果需要调整数量(需在FIXED模式才有意义),请自行调用线程池start函数输入线程数量
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
    std::any result_(const Model &) const;
    std::any result_for_(const std::chrono::nanoseconds &) const;
    std::any Return_() const;
#if _LIBCPP_STD_VER >= 20
    std::binary_semaphore &operator()(std::nullptr_t) const;
#else
    Xbinary_Semaphore &operator()(std::nullptr_t) const;
#endif
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
