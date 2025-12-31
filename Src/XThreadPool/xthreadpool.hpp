#ifndef X_THREADPOOL2_HPP
#define X_THREADPOOL2_HPP

#include <XThreadPool/xrunnable.hpp>
#include <XHelper/xtypetraits.hpp>
#include <XHelper/xcallablehelper.hpp>
#include <XMemory/xmemory.hpp>

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
using XThreadPoolPtr = std::shared_ptr<XThreadPool>;

class X_CLASS_EXPORT XThreadPoolData {
    X_DISABLE_COPY_MOVE(XThreadPoolData)
public:
    XThreadPool * m_x_ptr{};
protected:
    constexpr XThreadPoolData() = default;
public:
    virtual ~XThreadPoolData() = default;
};

class X_CLASS_EXPORT XThreadPool final
    : public std::enable_shared_from_this<XThreadPool>
    , XTwoPhaseConstruction<XThreadPool>
{
    X_DECLARE_PRIVATE(XThreadPool)
    X_TWO_PHASE_CONSTRUCTION_CLASS

    std::unique_ptr<XThreadPoolData> m_d_ptr_{};

    struct XTemporaryTasksFactory;

    template<typename ...Args>
    class XTemporaryTasks final: public XRunnable<Const> {
        enum class Private_{};
        friend struct XTemporaryTasksFactory;
        using invoker_t = Invoker<Args...>;
        mutable invoker_t m_invoker_{};
        constexpr std::any run() const override {
            if constexpr (std::is_void_v<typename invoker_t::result_t>) {
                m_invoker_();
                return {};
            }else {
                return m_invoker_();
            }
        }
    public:
        explicit constexpr XTemporaryTasks(Private_,Args && ...args)
            : m_invoker_{ XCallableHelper::createInvoker(std::forward<Args>(args)...)  }
        {}
        ~XTemporaryTasks() override = default;
    };

    struct XTemporaryTasksFactory final {
        XTemporaryTasksFactory() = delete;
        template<typename ...Args>
        static constexpr auto create(Args && ...args) noexcept {
            using TemporaryTasks = XTemporaryTasks<Args...>;
            return makeShared<TemporaryTasks>(typename TemporaryTasks::Private_{},std::forward<Args>(args)...);
        }
    };

public:
    enum class Mode {FIXED,/*固定线程数模式*/CACHE /*动态线程数*/};

    /// @return 返回CPU线程数量
    static unsigned cpuThreadsCount();

    /// 启动线程池,多次调用和在线程池内部线程调用无效,在CACHE模式下,默认线程数量无效
    /// @param threadSize
    [[maybe_unused]] void start(XSize_t threadSize = cpuThreadsCount());

    ///停止线程池,线程池内部的线程调用无效
    void stop() ;

    /// 检查线程池是否运行
    /// @return ture or false
    [[maybe_unused]] [[nodiscard]] bool isRunning() const noexcept;

    /// 加入任务,如果没有在此函数前显式调用start,本函数会调用start启动
    /// 对于加入失败的任务,会对任务设置一个空的返回值以防止外部被阻塞
    /// 本函数如果在线程池管理的线程调用是无效的,不会导致程序崩溃
    /// 支持临时任务,生命周期需自行管理,支持全局函数(静态和非静态)、仿函数、Lambda、成员函数(静态和非静态)、函数包装器
    /// @tparam Args
    /// @param args
    /// @return task对象
    template<typename... Args>
    [[maybe_unused]] constexpr auto runnableJoin(Args && ...args) {

        using First_t [[maybe_unused]] = std::tuple_element_t<0,std::tuple<Args...>>;

        if constexpr (is_smart_pointer_v<std::decay_t<First_t>>){

            using Derived_t = std::decay_t<decltype(std::declval<First_t>().operator*())>;

            static_assert(std::is_base_of_v<XAbstractRunnable,Derived_t>,"Derived_t no base of XAbstractTask2");

            return appendHelper(std::forward<Args>(args)...);
        }else{
            return appendHelper(XTemporaryTasksFactory::create(std::forward<Args>(args)...));
        }
    }

    /// 模式设置,线程池启动后设置无效
    /// @param mode
    [[maybe_unused]] void setMode(Mode mode) noexcept;

    /// @return 获取当前线程池模式
    [[maybe_unused]][[nodiscard]] Mode getMode() const noexcept;

    /// 线程数阈值设置,线程池启动后设置无效
    /// @param num
    [[maybe_unused]] void setThreadsSizeThreshold(XSize_t num) noexcept;

    /// @return 线程池线程数量阈值
    [[maybe_unused]] [[nodiscard]] XSize_t getThreadsSizeThreshold() const noexcept;

    /// 任务数阈值设置,线程池启动后设置无效
    /// @param num
    [[maybe_unused]] void setTasksSizeThreshold(XSize_t num) noexcept;

    /// @return 线程池任务数量阈值
    [[maybe_unused]] [[nodiscard]] XSize_t getTasksSizeThreshold() const noexcept;

    /// @return 空闲线程数量
    [[maybe_unused]] [[nodiscard]] XSize_t idleThreadsSize() const noexcept;

    /// @return 当前线程数量
    [[maybe_unused]] [[nodiscard]] XSize_t currentThreadsSize() const noexcept;

    /// @return 忙线程数量
    [[maybe_unused]] [[nodiscard]] XSize_t busyThreadsSize() const noexcept;

    /// @return 当前任务数量
    [[maybe_unused]] [[nodiscard]] XSize_t currentTasksSize() const noexcept;

    /// 设置线程池在CACHE模式下线程等待任务的时间,如果线程超时则退出,默认是60s
    /// 只在CACHE模式下有效
    /// 线程池启动后设置无效
    /// @param seconds 单位是秒
    [[maybe_unused]] void setThreadTimeout(XSize_t seconds) noexcept;

    /// 创建线程池对象,默认模式为FIXED
    /// @param mode
    /// @return 线程池对象
    [[maybe_unused]] [[nodiscard]] static XThreadPoolPtr create(Mode mode = Mode::FIXED) noexcept;

    ~XThreadPool();

    X_DISABLE_COPY_MOVE(XThreadPool)

private:
    explicit XThreadPool();
    bool construct_();
    XAbstractRunnablePtr appendHelper( XAbstractRunnablePtr ) ;
};

[[maybe_unused]] X_API void sleep_for_ns(XSize_t ns);

[[maybe_unused]] X_API void sleep_for_us(XSize_t us);

[[maybe_unused]] X_API void sleep_for_ms(XSize_t ms);

[[maybe_unused]] X_API void sleep_for_s(XSize_t s);

[[maybe_unused]] X_API void sleep_for_mins(XSize_t mins);

[[maybe_unused]] X_API void sleep_for_hours(XSize_t h);

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
