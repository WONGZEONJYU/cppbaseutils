#ifndef X_THREADPOOL2_HPP
#define X_THREADPOOL2_HPP

#include <XThreadPool/xrunnable.hpp>
#include <XHelper/xtypetraits.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

#if defined(__LP64__) || defined(_WIN64)
using XSize_t = xint64;
using XUSize_t = xuint64;
#else
using XSize_t = xint32;
using XUSize_t = xuint32;
#endif

class XThreadPool;
class XThreadPoolPrivate;
using XThreadPool_Ptr = std::shared_ptr<XThreadPool>;

class XThreadPoolData {
    X_DISABLE_COPY_MOVE(XThreadPoolData)
protected:
    XThreadPoolData() = default;
public:
    virtual ~XThreadPoolData() = default;
};

class XThreadPool final : public std::enable_shared_from_this<XThreadPool> {
    enum class Private{};
    X_DECLARE_PRIVATE(XThreadPool)

    using XThreadPoolData_Ptr = std::unique_ptr<XThreadPoolData>;
    mutable XThreadPoolData_Ptr m_d_ptr_{};

    template<typename ...Args>
    class XTemporaryTasksImpl;
    class XTemporaryTasksFactory;

    class XTemporaryTasksBase final {
        enum class Private{};
        template<typename... >
        friend class XTemporaryTasksImpl;
        friend class XTemporaryTasksFactory;
    public:
        XTemporaryTasksBase() = delete;
    };

    template<typename ...Args>
    class XTemporaryTasksImpl final: public XRunnable<Const> {

        using decayed_tuple_ = std::tuple<std::decay_t<Args>...>;
        mutable decayed_tuple_ m_tuple_{};

        template<typename>
        struct result;

        template<typename Fn_,typename ...Args_>
        struct result<std::tuple<Fn_,Args_...>> : std::invoke_result<Fn_,Args_...> {};

        using result_t = typename result<decayed_tuple_>::type;

        std::any run() const override {
            using indices = std::make_index_sequence<std::tuple_size_v<decayed_tuple_>>;
            if constexpr (std::is_void_v<result_t>){
                call(indices{});
                return {};
            }else{
                return (std::move(call(indices{})));
            }
        }

        template<size_t ...I>
        result_t call(std::index_sequence<I...>) const {
            return std::invoke(std::get<I>(std::forward<decayed_tuple_>(m_tuple_))...);
        }

    public:
        explicit XTemporaryTasksImpl(XTemporaryTasksBase::Private,Args && ...args):m_tuple_{std::forward<Args>(args)...}{}
        ~XTemporaryTasksImpl() override = default;
    };

    class XTemporaryTasksFactory final {
    public:
        XTemporaryTasksFactory() = delete;

        template<typename ...Args>
        static auto create(Args && ...args) {
            try{
                return std::make_shared<XTemporaryTasksImpl<Args...>>(XTemporaryTasksBase::Private{},std::forward<Args>(args)...);
            }catch (const std::exception &){
                return std::shared_ptr<XTemporaryTasksImpl<Args...>>{};
            }
        }
    };

    XAbstractRunnable_Ptr runnableJoin_(const XAbstractRunnable_Ptr &task);

public:
    enum class Mode {FIXED,/*固定线程数模式*/CACHE /*动态线程数*/};

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
    /// 支持临时任务,生命周期需自行管理,支持全局函数(静态和非静态)、仿函数、Lambda、成员函数(静态和非静态)、函数包装器
    /// @tparam Args
    /// @param args
    /// @return task对象
    template<typename... Args>
    [[maybe_unused]] auto runnableJoin(Args && ...args) {

        using First_t [[maybe_unused]] = std::tuple_element_t<0,std::tuple<Args...>>;

        if constexpr (is_smart_pointer_v<std::decay_t<First_t>>){

            using Derived_t = std::decay_t<decltype(std::declval<First_t>().operator*())>;

            static_assert(std::is_base_of_v<XAbstractRunnable,Derived_t>,"Derived_t no base of XAbstractTask2");

            return runnableJoin_(std::forward<Args>(args)...);
        }else{
            return runnableJoin_(XTemporaryTasksFactory::create(std::forward<Args>(args)...));
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
    [[maybe_unused]] [[nodiscard]] static XThreadPool_Ptr create(const Mode &mode = FixedModel);

    explicit XThreadPool(Private,const Mode &,XThreadPoolData_Ptr &&);

    ~XThreadPool();

    X_DISABLE_COPY_MOVE(XThreadPool)
};

[[maybe_unused]] void sleep_for_ns(const XSize_t& ns);

[[maybe_unused]] void sleep_for_us(const XSize_t& us);

[[maybe_unused]] void sleep_for_ms(const XSize_t& ms);

[[maybe_unused]] void sleep_for_s(const XSize_t& s);

[[maybe_unused]] void sleep_for_mins(const XSize_t& mins);

[[maybe_unused]] void sleep_for_hours(const XSize_t& h);

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
