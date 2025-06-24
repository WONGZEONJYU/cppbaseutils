#ifndef X_THREADPOOL2_HPP
#define X_THREADPOOL2_HPP

#include <XThreadPool/xabstracttask2.hpp>
#include <XHelper/xtypetraits.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

#if defined(__LP64__)
using XSize_t = int64_t;
using XUSize_t = uint64_t;
#else
using XSize_t = int32_t;
using XUSize_t = uint64_t;
#endif

class XThreadPool2;
using XThreadPool2_Ptr = std::shared_ptr<XThreadPool2>;

class XThreadPool2 final : public std::enable_shared_from_this<XThreadPool2> {
    class XThreadPool2Private;
    class XThreadPool2Data {
        X_DISABLE_COPY_MOVE(XThreadPool2Data)
    protected:
        XThreadPool2Data() = default;
    public:
        virtual ~XThreadPool2Data() = default;
    };
    using XThreadPool2Data_Ptr = std::unique_ptr<XThreadPool2Data>;
    mutable XThreadPool2Data_Ptr m_d_ptr_{};
    X_DECLARE_PRIVATE(XThreadPool2)

    class XBaseTemporaryTasks {
    protected:
        XBaseTemporaryTasks() = default;
        virtual ~XBaseTemporaryTasks() = default;
        enum class Private{};
    };

    template<typename Fn,typename ...Args>
    class XTemporaryTasksImpl final: public XAbstractTask2,XBaseTemporaryTasks {

        using ReturnType = std::invoke_result_t<std::decay_t<Fn>,std::decay_t<Args>...>;
        using decayed_Tuple_ = std::tuple<std::decay_t<Fn>,std::decay_t<Args>...>;
        mutable decayed_Tuple_ m_tuple_{};

        std::any run() const override {
            using indices = std::make_index_sequence<std::tuple_size_v<decayed_Tuple_>>;
            const auto& call{*this};
            if constexpr (std::is_same_v<ReturnType,void>){
                call(indices{});
                return {};
            }else{
                return call(indices{});
            }
        }

        template<size_t ...I>
        ReturnType operator()(std::index_sequence<I...>) const{
            return std::invoke(std::get<I>(std::forward<decayed_Tuple_>(m_tuple_))...);
        }

    public:
        explicit XTemporaryTasksImpl(Private,Fn && fn,Args && ...args):XAbstractTask2(CONST_RUN),
        m_tuple_{std::forward<std::decay_t<Fn>>(fn),std::forward<std::decay_t<Args>>(args)...}{}
        ~XTemporaryTasksImpl() override = default;
    };

    class XTemporaryTasksFactory final : XBaseTemporaryTasks {
    public:
        XTemporaryTasksFactory() = delete;

        template<typename ...Args_>
        using XTemporaryTasksImpl_Ptr = std::shared_ptr<XTemporaryTasksImpl<Args_...>>;

        template<typename ...Args_>
        static XTemporaryTasksImpl_Ptr<Args_...> create(Args_ && ...args) {
            try{
                return std::make_shared<XTemporaryTasksImpl<Args_...>>(Private{},std::forward<Args_>(args)...);
            }catch (const std::exception &){
                return {};
            }
        }
    };

    XAbstractTask2_Ptr taskJoin_(const XAbstractTask2_Ptr &task);

public:
    enum class Mode {
        FIXED,/*固定线程数模式*/
        CACHE /*动态线程数*/
    };

    static constexpr auto FixedModel{Mode::FIXED},CacheModel{Mode::CACHE};

    /// @return 返回CPU线程数量
    static unsigned cpuThreadsCount();

    /// 启动线程池,多次调用和在线程池内部线程调用无效,在CACHE模式下,默认线程数量无效
    /// @param threadSize
    [[maybe_unused]] void start(const XSize_t &threadSize = cpuThreadsCount());

    ///停止线程池,线程池内部的线程调用无效
    void stop() const;

    /// 检查线程池是否运行
    /// @return ture or false
    [[maybe_unused]] [[nodiscard]] bool isRunning() const;

    /// 加入任务,如果没有在此函数前显式调用start,本函数会调用start启动
    /// 对于加入失败的任务,会对任务设置一个空的返回值以防止外部被阻塞
    /// 本函数如果在线程池管理的线程调用是无效的,不会导致程序崩溃
    /// 临时任务,生命周期需自行管理,支持全局函数(静态和非静态)、仿函数、Lambda、成员函数(静态和非静态)、函数包装器
    /// @tparam Args
    /// @param args
    /// @return task对象
    template<typename... Args>
    [[maybe_unused]] auto taskJoin(Args && ...args) {

        using First_t [[maybe_unused]] = std::tuple_element_t<0,std::tuple<Args...>>;

        if constexpr (is_smart_pointer_v<std::decay_t<First_t>>){

            using Derived_t = std::decay_t<decltype(std::declval<First_t>().operator*())>;

            static_assert(std::is_base_of_v<XAbstractTask2,Derived_t>,"Derived_t no base of XAbstractTask2");

            return taskJoin_(std::forward<Args>(args)...);
        }else{
            return taskJoin_(XTemporaryTasksFactory::create(std::forward<Args>(args)...));
        }
    }

    /// 模式设置,线程池启动后设置无效
    /// @param mode
    [[maybe_unused]] void setMode(const Mode &mode) const;

    /// @return 获取当前线程池模式
    [[maybe_unused]][[nodiscard]] Mode getMode() const;

    /// 线程数阈值设置,线程池启动后设置无效
    /// @param num
    [[maybe_unused]] void setThreadsSizeThreshold(const XSize_t &num) const;

    /// @return 线程池线程数量阈值
    [[maybe_unused]] [[nodiscard]] XSize_t getThreadsSizeThreshold() const;

    /// 任务数阈值设置,线程池启动后设置无效
    /// @param num
    [[maybe_unused]] void setTasksSizeThreshold(const XSize_t &num) const;

    /// @return 线程池任务数量阈值
    [[maybe_unused]] [[nodiscard]] XSize_t getTasksSizeThreshold() const;

    /// @return 空闲线程数量
    [[maybe_unused]] [[nodiscard]] XSize_t idleThreadsSize() const;

    /// @return 当前线程数量
    [[maybe_unused]] [[nodiscard]] XSize_t currentThreadsSize() const;

    /// @return 忙线程数量
    [[maybe_unused]] [[nodiscard]] XSize_t busyThreadsSize() const;

    /// @return 当前任务数量
    [[maybe_unused]] [[nodiscard]] XSize_t currentTasksSize() const;

    /// 设置线程池在CACHE模式下线程等待任务的时间,如果线程超时则退出,默认是60s
    /// 只在CACHE模式下有效
    /// 线程池启动后设置无效
    /// @param seconds 单位是秒
    [[maybe_unused]] void setThreadTimeout(const XSize_t & seconds) const;

    /// 创建线程池对象,默认模式为FIXED
    /// @param mode
    /// @return 线程池对象
    [[maybe_unused]] [[nodiscard]] static XThreadPool2_Ptr create(const Mode &mode = FixedModel);

    explicit XThreadPool2(const Mode &,XThreadPool2Data_Ptr &&);

    ~XThreadPool2();

    X_DISABLE_COPY_MOVE(XThreadPool2)
};

[[maybe_unused]] void sleep_for_ns(const XSize_t& ns);

[[maybe_unused]] void sleep_for_us(const XSize_t& us);

[[maybe_unused]] void sleep_for_ms(const XSize_t& ms);

[[maybe_unused]] void sleep_for_s(const XSize_t& s);

[[maybe_unused]] void sleep_for_mins(const XSize_t& mins);

[[maybe_unused]] void sleep_for_hours(const XSize_t& h);

[[maybe_unused]] void sleep_until_ns(const XSize_t& ns);

[[maybe_unused]] void sleep_until_us(const XSize_t& us);

[[maybe_unused]] void sleep_until_ms(const XSize_t& ms);

[[maybe_unused]] void sleep_until_s(const XSize_t& s);

[[maybe_unused]] void sleep_until_mins(const XSize_t& mins);

[[maybe_unused]] void sleep_until_hours(const XSize_t& h);

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
