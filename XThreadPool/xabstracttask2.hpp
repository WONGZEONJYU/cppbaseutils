#ifndef X_ABSTRACT_TASK2_HPP
#define X_ABSTRACT_TASK2_HPP

#include <XHelper/xhelper.hpp>
#include <any>
#include <memory>
#include <functional>
#include <iostream>
#include <XAtomic/xatomic.hpp>
#include <XHelper/xtypetraits.hpp>

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
    enum class Model {
        BLOCK,///@brief 阻塞
        NONBLOCK,///@brief 非阻塞
    };

    enum class FuncVer{
        CONST,
        NON_CONST,
    };

    class XAbstractTask2Private;
    X_DECLARE_PRIVATE(XAbstractTask2)

    class XAbstractTask2Data {
        X_DISABLE_COPY_MOVE(XAbstractTask2Data)
        friend class XAbstractTask2;
        XAbstractTask2 * m_x_ptr_{};
#if _LIBCPP_STD_VER >= 20
        std::binary_semaphore&
#else
        Xbinary_Semaphore&
#endif
        operator()() const;
        [[nodiscard]] std::any getResult() const;
        [[nodiscard]] std::any result_(const Model &) const;
        [[nodiscard]] std::any result_for_(const std::chrono::nanoseconds &) const;
    protected:
        XAbstractTask2Data() = default;
    public:
        virtual ~XAbstractTask2Data() = default;
    };

    mutable std::shared_ptr<XAbstractTask2Data> m_d_ptr_{};

public:
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
        const auto v{m_d_ptr_->result_(model_)};
        return v.has_value() ? std::move(std::any_cast<Ty>(v)) : Ty{};
    }

    /// 带超时等待返回值
    /// @tparam Ty
    /// @tparam Rep_
    /// @tparam Period_
    /// @param rel_time
    /// @return Ty类型数据
    template<typename Ty,typename Rep_,typename Period_>
    [[maybe_unused]] [[nodiscard]] Ty result(const std::chrono::duration<Rep_,Period_> &rel_time) const noexcept(false) {
        const auto v{m_d_ptr_->result_for_(std::chrono::duration_cast<std::chrono::nanoseconds>(rel_time))};
        return v.has_value() ? std::move(std::any_cast<Ty>(v)) : Ty{};
    }

    /// 带指定时间等候返回值
    /// @tparam Ty
    /// @tparam Clock_
    /// @param abs_time_
    /// @return Ty类型
    template<typename Ty,typename Clock_,typename Duration_>
    [[maybe_unused]] [[nodiscard]] Ty result(const std::chrono::time_point<Clock_,Duration_> & abs_time_) const noexcept(false) {

        constexpr std::string_view selfname{__PRETTY_FUNCTION__};
#if 0
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

        if (const auto& until{*m_d_ptr_};
            !until().try_acquire_until(abs_time_)){
            std::cerr << selfname << " tips: timeout\n" << std::flush;
            return Ty{};
        }

        const auto v{m_d_ptr_->getResult()};
        return v.has_value() ? std::move(std::any_cast<Ty>(v)) : Ty{};
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

    /// 检查线程池是否运行
    /// @return  ture or false
    [[maybe_unused]] [[nodiscard]] bool is_running_() const;

    virtual ~XAbstractTask2() = default;

private:
    explicit XAbstractTask2(const FuncVer& is_OverrideConst);
protected:
    static constexpr auto NON_CONST_RUN{FuncVer::NON_CONST},CONST_RUN{FuncVer::CONST};
    /// 如果重载的是 virtual std::any run() const;
    /// is_OverrideConst = CONST_RUN
    /// 如果是virtual std::any run();
    /// is_OverrideConst = NON_CONST_RUN
    /// 填错会在运行时出现提示
    /// @param is_OverrideConst

    template<typename T>
    explicit XAbstractTask2(T *):
    XAbstractTask2(virtual_override_checker<XAbstractTask2,T>::derived_func_is_const ? CONST_RUN : NON_CONST_RUN){

    }

    ///响应责任链的请求,需开发者自行重写
    /// @param arg
    [[maybe_unused]] virtual void responseHandler(const std::any &arg) {(void )arg;}

    /// 开发者注意,重写那个版本需在构造函数写明
    /// @return 任意类型
    virtual std::any run() {
        std::cerr <<
            __PRETTY_FUNCTION__ << " tips: You did not rewrite this function, "
            "please pay attention to whether your constructor parameters are filled in incorrectly\n";
        return {};
    }

    virtual std::any run () const {
        std::cerr <<
            __PRETTY_FUNCTION__ << " tips: You did not rewrite this function, "
            "please pay attention to whether your constructor parameters are filled in incorrectly\n";
        return {};
    }

private:
    friend class XThreadPool2;
    void operator()();
    void operator()() const;
    bool is_OverrideConst() const;
    void set_exit_function_(std::function<bool()> &&) const;
    void resetOccupy_() const;
    XAtomicPointer<const void> &Owner() const;
    X_DEFAULT_COPY_MOVE(XAbstractTask2)
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
