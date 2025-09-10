#ifndef XUTILS_XLOG_HPP
#define XUTILS_XLOG_HPP 1

#include <XHelper/xhelper.hpp>
#ifdef _WIN32
#include <windows.h>
#endif

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

/**
 * @brief 日志级别枚举
 * 使用_LEVEL后缀避免与系统平台宏冲突
 */
enum class LogLevel : uint8_t {
    TRACE_LEVEL = 0,
    DEBUG_LEVEL = 1,
    INFO_LEVEL = 2,
    WARN_LEVEL = 3,
    ERROR_LEVEL = 4,
    FATAL_LEVEL = 5
};

/**
 * @brief 日志输出类型
 */
enum class LogOutput : uint8_t {
    CONSOLE = 1 << 0,  // 控制台输出
    FILE = 1 << 1,     // 文件输出
    BOTH = CONSOLE | FILE  // 同时输出到控制台和文件
};

// 启用位运算操作符
constexpr LogOutput operator| (LogOutput const & lhs, LogOutput const & rhs) noexcept
{return static_cast<LogOutput>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));}

constexpr LogOutput operator& (LogOutput const & lhs, LogOutput const & rhs) noexcept
{return static_cast<LogOutput>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));}

/**
 * @brief 源代码位置信息（C++17兼容版本）
 */
struct SourceLocation final {

    const char* m_fileName{},
            * m_functionName{};
    std::uint32_t m_line{};

    SourceLocation() = default;

    constexpr SourceLocation(const char * const file
                            ,const char * const function
                            ,uint32_t const line_num) noexcept
    :m_fileName(file),m_functionName(function),m_line(line_num){}

    // 创建当前位置的便利函数
    static constexpr SourceLocation current(const char* const file = {},
                                          const char * const function = {},
                                          std::uint32_t const line_num = {}) noexcept {
        return{file,function, line_num};
    }
};

/**
 * @brief 日志消息结构
 */
struct LogMessage final {

    LogLevel level{};
    std::string timestamp{}
        , thread_id{}
        , file{}
        , function{}
        , message{};
    std::uint32_t line{};

    LogMessage() = default;

    LogMessage(LogLevel const lv, std::string ts, std::string tid,
               std::string f, std::uint32_t const l, std::string func, std::string msg) noexcept
        : level(lv), timestamp(std::move(ts)), thread_id(std::move(tid))
        , file(std::move(f)), function(std::move(func))
        , message(std::move(msg)), line(l) {}
};

/**
 * @brief 崩溃处理器接口
 */
class X_CLASS_EXPORT ICrashHandler : public std::enable_shared_from_this<ICrashHandler>
        ,public XHelperClass<ICrashHandler>
{
    X_HELPER_CLASS
public:
    virtual ~ICrashHandler() = default;
    virtual void onCrash(std::string_view const & crash_info) = 0;
};

class XLog;
class XLogPrivate;
class XLogData {
protected:
    XLogData() = default;
public:
    virtual ~XLogData() = default;
    XLog * m_x_ptr_{};
};

[[maybe_unused]] [[nodiscard]] X_API XLog * XlogHandle() noexcept;

/**
 * @brief 线程安全的异步日志系统
 * 
 * 特性：
 * - 线程安全的异步日志记录
 * - 支持控制台和文件输出
 * - 支持日志级别过滤
 * - 支持崩溃诊断和堆栈跟踪
 * - 支持自定义日志格式
 * - 支持日志文件轮转
 * - 优雅关闭机制
 * - 现代C++特性优化
 */
class X_CLASS_EXPORT XLog final : XSingleton<XLog> {
    X_HELPER_CLASS
    X_DECLARE_PRIVATE_D(m_d_ptr,XLog)
    using CrashHandlerPtr_ = std::shared_ptr<ICrashHandler>;
    std::unique_ptr<XLogData> m_d_ptr{};
public:
    using CrashHandlerPtr = CrashHandlerPtr_;
    using TimePoint [[maybe_unused]] = std::chrono::system_clock::time_point;

    /**
     * @brief 设置日志级别
     * @param level 最低日志级别
     */
    void setLogLevel(LogLevel const& level) noexcept;

    /**
     * @brief 获取当前日志级别
     * @return 当前日志级别
     */
    [[nodiscard]] LogLevel getLogLevel() const noexcept;
    
    /**
     * @brief 设置日志输出方式
     * @param output 输出方式（控制台、文件或两者）
     */
    void setOutput(LogOutput const & output) noexcept;
    
    /**
     * @brief 设置日志文件配置
     * @param base_name 基础文件名（不包含扩展名和时间戳）
     * @param directory 日志文件目录
     * @param max_size_mb 单个日志文件最大大小（MB），默认5MB
     * @param retention_days 日志保存天数，默认7天
     */
    void setLogFileConfig(std::string_view const & base_name = "application", 
                         std::string_view const & directory = "logs",
                         std::size_t max_size_mb = 5, 
                         int retention_days = 7);

    /**
     * @brief 获取当前日志文件路径
     * @return 当前日志文件的完整路径
     */
    [[maybe_unused]] [[nodiscard]] std::string getCurrentLogFile() const;

    /**
     * @brief 清理过期的日志文件
     */
    void cleanupOldLogFiles() const noexcept;

    /**
     * @brief 启用/禁用控制台彩色输出
     * @param enable 是否启用彩色输出
     */
    void setColorOutput(bool enable) noexcept;

    /**
     * @brief 设置异步队列大小
     * @param size 队列大小，0表示无限制
     */
    void setAsyncQueueSize(std::size_t size) noexcept;

    /**
     * @brief 启用崩溃诊断
     * @param enable 是否启用崩溃诊断
     */
    void enableCrashDiagnostics(bool enable = true);

    /**
     * @brief 设置崩溃处理器
     * @param handler 崩溃处理器
     */
    void setCrashHandler(CrashHandlerPtr && handler);

    /**
     * @brief 记录日志（现代化接口）
     * @param level 日志级别
     * @param message 日志消息
     * @param location 源代码位置信息
     */
    void log(LogLevel const & level, std::string_view const & message,
             SourceLocation const & location = {});
    
    /**
     * @brief 格式化记录日志
     * @tparam Args 参数类型
     * @param level 日志级别
     * @param format_str 格式字符串
     * @param location 源代码位置信息
     * @param args 格式参数
     */
    template<typename... Args>
    inline void logFormat(LogLevel const & level, const char * const format_str,
              SourceLocation const & location, Args &&... args)
    {
        if (!shouldLog(level)) { return; }

        try {
            // 使用简单的字符串格式化（C++17兼容）
            std::ostringstream oss{};
            formatImpl(oss, format_str, std::forward<Args>(args)...);
            log(level, oss.str(), location);
        } catch (const std::exception& e) {
            // 格式化失败时记录错误
            log(LogLevel::ERROR_LEVEL
                , (std::ostringstream{} << "Log format error: " << e.what()).str()
                , location);
        }
    }

    /**
     * @brief 刷新所有待处理的日志
     */
    void flush();
    
    /**
     * @brief 等待所有日志处理完成
     * @param timeout 超时时间（毫秒），0表示无限等待
     * @return 是否在超时前完成
     */
    [[maybe_unused]] [[nodiscard]] bool waitForCompletion(std::chrono::milliseconds const & timeout = std::chrono::milliseconds::zero());
    
    /**
     * @brief 获取当前队列大小
     * @return 队列中待处理的日志数量
     */
    [[maybe_unused]] [[nodiscard]] std::size_t getQueueSize() const;
    
    /**
     * @brief 检查是否应该记录指定级别的日志
     * @param level 日志级别
     * @return 是否应该记录
     */
    [[nodiscard]] bool shouldLog(LogLevel const & level) const noexcept;

    /**
     * @brief 获取日志级别名称
     * @param level 日志级别
     * @return 级别名称
     */
    [[nodiscard]] inline static constexpr std::string_view getLevelName(LogLevel const & level) noexcept {
        switch (level) {
            case LogLevel::TRACE_LEVEL: return "TRACE";
            case LogLevel::DEBUG_LEVEL: return "DEBUG";
            case LogLevel::INFO_LEVEL:  return "INFO";
            case LogLevel::WARN_LEVEL:  return "WARN";
            case LogLevel::ERROR_LEVEL: return "ERROR";
            case LogLevel::FATAL_LEVEL: return "FATAL";
            default: return "UNKNOWN";
        }
    }

    /**
     * @brief 获取当前时间戳字符串
     * @return 时间戳字符串
     */
    [[nodiscard]] static std::string getCurrentTimestamp();
    
    /**
     * @brief 获取当前线程ID字符串
     * @return 线程ID字符串
     */
    [[nodiscard]] static std::string getCurrentThreadId();
    
    /**
     * @brief 获取堆栈跟踪信息
     * @param skip_frames 跳过的栈帧数量
     * @return 堆栈跟踪字符串
     */
    [[nodiscard]] static std::string getStackTrace(int skip_frames = 1);

    static void xlogHelper(LogLevel const &
            ,std::string_view const &
            ,SourceLocation const &
            ,bool = false);

    template<typename ...Args>
    inline static void xlogFormatHelper(LogLevel const & level
                                        ,const char * const format
                                        ,SourceLocation const & location
                                        ,bool const b
                                        ,Args && ...args) noexcept {

        if ( auto const logger{instance()}
            ;logger && logger->shouldLog(level)) {
            logger->logFormat(level,format,location,std::forward< Args >(args)...);
            if (b){logger->flush();}
        }
    }

private:
    XLog();
    ~XLog();
    bool construct_();
    static auto instance() noexcept -> XLog *;
    // 格式化辅助函数 - 使用标准printf风格格式化
    template<typename... Args>
    inline static void formatImpl(std::ostringstream & , const char* , Args &&...);
    X_DISABLE_COPY_MOVE(XLog)
    friend X_API XLog * XlogHandle() noexcept;
};

template<typename... Args>
inline void XLog::formatImpl(std::ostringstream & oss
                            , const char * const format
                            , Args &&... args)
{
    using BufferType = std::vector<char,XPrivate::Allocator_<char>>;

    if constexpr ( sizeof...(args) > 0 ) { //有参数时使用snprintf进行格式化
        constexpr auto PredictSize {4096};
        BufferType buffer(PredictSize,{});
        if (auto const result { std::snprintf(buffer.data(), buffer.size(), format, args...) }
            ; result > 0 && static_cast<std::size_t>(result) < buffer.size())
        {
            oss << buffer.data();
        } else if (result > 0) {
            // 缓冲区太小，需要更大的缓冲区
            BufferType largerBuffer(result + 1,{});
            std::snprintf(largerBuffer.data(), result + 1, format, args...);
            oss << largerBuffer.data();
        } else {
            // 格式化失败，输出原始格式字符串
            oss << format;
        }
    } else {
        // 无参数时直接输出格式字符串
        oss << format;
    }
}

// 现代化的便利宏定义 - 使用辅助宏减少重复代码
#define XLOG_IMPL(level, msg)       \
    XUtils::XLog::xlogHelper(level  \
    ,msg                            \
    ,XUtils::SourceLocation::current(__FILE__, FUNC_SIGNATURE, __LINE__))

#define XLOG_FATAL_IMPL(level, msg) \
    XUtils::XLog::xlogHelper(level  \
    ,msg                            \
    ,XUtils::SourceLocation::current(__FILE__, FUNC_SIGNATURE, __LINE__),true)

#define XLOG_TRACE(msg) XLOG_IMPL(XUtils::LogLevel::TRACE_LEVEL, msg)
#define XLOG_DEBUG(msg) XLOG_IMPL(XUtils::LogLevel::DEBUG_LEVEL, msg)
#define XLOG_INFO(msg)  XLOG_IMPL(XUtils::LogLevel::INFO_LEVEL, msg)
#define XLOG_WARN(msg)  XLOG_IMPL(XUtils::LogLevel::WARN_LEVEL, msg)
#define XLOG_ERROR(msg) XLOG_IMPL(XUtils::LogLevel::ERROR_LEVEL, msg)
#define XLOG_FATAL(msg) XLOG_FATAL_IMPL(XUtils::LogLevel::FATAL_LEVEL, msg)

// 格式化日志宏 - 使用辅助宏来处理可变参数
#define XLOG_FORMAT_IMPL(level, fmt, ...) \
    XUtils::XLog::xlogFormatHelper(level,fmt \
        ,XUtils::SourceLocation::current(__FILE__, FUNC_SIGNATURE, __LINE__) \
        , false __VA_OPT__(, ) __VA_ARGS__ )

#define XLOG_FATAL_FORMAT_IMPL(level, fmt, ...) \
    XUtils::XLog::xlogFormatHelper(level,fmt \
        ,XUtils::SourceLocation::current(__FILE__, FUNC_SIGNATURE, __LINE__) \
        , true __VA_OPT__(, ) __VA_ARGS__ )

#define XLOGF_TRACE(fmt, ...) XLOG_FORMAT_IMPL(XUtils::LogLevel::TRACE_LEVEL, fmt __VA_OPT__(, ) __VA_ARGS__)
#define XLOGF_DEBUG(fmt, ...) XLOG_FORMAT_IMPL(XUtils::LogLevel::DEBUG_LEVEL, fmt __VA_OPT__(, ) __VA_ARGS__)
#define XLOGF_INFO(fmt, ...)  XLOG_FORMAT_IMPL(XUtils::LogLevel::INFO_LEVEL, fmt __VA_OPT__(, ) __VA_ARGS__)
#define XLOGF_WARN(fmt, ...)  XLOG_FORMAT_IMPL(XUtils::LogLevel::WARN_LEVEL, fmt __VA_OPT__(, ) __VA_ARGS__)
#define XLOGF_ERROR(fmt, ...) XLOG_FORMAT_IMPL(XUtils::LogLevel::ERROR_LEVEL, fmt __VA_OPT__(, ) __VA_ARGS__)
#define XLOGF_FATAL(fmt, ...) XLOG_FATAL_FORMAT_IMPL(XUtils::LogLevel::FATAL_LEVEL, fmt __VA_OPT__(, ) __VA_ARGS__)

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
