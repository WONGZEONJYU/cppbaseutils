#ifndef XUTILS_XLOG_HPP
#define XUTILS_XLOG_HPP 1

#include <XHelper/xhelper.hpp>
#include <string>
#include <string_view>
#include <memory>
#include <atomic>
#include <chrono>
#include <fstream>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <queue>
#include <condition_variable>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

/**
 * @brief 日志级别枚举
 * 使用_LEVEL后缀避免与系统平台宏冲突
 */
enum class LogLevel : std::uint8_t {
    TRACE_LEVEL = 0,
    DEBUG_LEVEL = 1,
    INFO_LEVEL = 2,
    WARN_LEVEL = 3,
    ERROR_LEVEL = 4,
    FATAL_LEVEL = 5
};

// 为了向后兼容和便于使用，提供简短别名（在没有宏冲突的平台上）
#ifndef TRACE
    [[maybe_unused]] inline constexpr auto TRACE {LogLevel::TRACE_LEVEL};
#endif
#ifndef DEBUG
    [[maybe_unused]] inline constexpr auto DEBUG {LogLevel::DEBUG_LEVEL};
#endif
#ifndef INFO
    [[maybe_unused]] inline constexpr auto INFO {LogLevel::INFO_LEVEL};
#endif
#ifndef WARN
    [[maybe_unused]] inline constexpr auto WARN {LogLevel::WARN_LEVEL};
#endif
#ifndef ERROR
    [[maybe_unused]] inline constexpr auto ERROR {LogLevel::ERROR_LEVEL};
#endif
#ifndef FATAL
    [[maybe_unused]] inline constexpr auto FATAL {LogLevel::FATAL_LEVEL};
#endif

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

    [[maybe_unused]] constexpr SourceLocation(const char * const file
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
class X_API ICrashHandler : public std::enable_shared_from_this<ICrashHandler>
        ,public XHelperClass<ICrashHandler>
{
    X_HELPER_CLASS
public:
    virtual ~ICrashHandler() = default;
    virtual void onCrash(std::string_view const & crash_info) = 0;
};

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
class X_API XLog final : public XSingleton<XLog> {
    X_HELPER_CLASS
    using CrashHandlerPtr_ = std::shared_ptr<ICrashHandler>;
    // 配置参数
    std::atomic<LogLevel> m_log_level_ {LogLevel::INFO_LEVEL};
    std::atomic<LogOutput> m_output_ {LogOutput::BOTH};
    std::atomic_bool m_color_output_{true}
    ,m_crash_diagnostics_{true};
    std::atomic_size_t m_max_queue_size_ {10000};

    // 文件相关
    std::string m_log_file_path_{};
    std::atomic_size_t m_max_file_size_{};
    std::atomic_int m_max_files_{5};
    std::unique_ptr<std::ofstream> m_file_stream_{};
    std::atomic_size_t m_current_file_size_{};

    // 异步处理
    std::queue<LogMessage> m_log_queue_{};
    mutable std::shared_mutex m_queue_mutex_{};
    std::condition_variable_any m_queue_cv_{};
    std::thread m_worker_thread_{};
    std::atomic_bool m_running_{}
    ,m_shutdown_requested_{};

    // 崩溃处理
    CrashHandlerPtr_ m_crash_handler_{};
    static inline XLog* s_instance_{};

    // 同步
    mutable std::shared_mutex m_config_mutex_{};
    mutable std::mutex m_file_mutex_{};

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
     * @brief 设置日志文件路径
     * @param filepath 日志文件路径
     * @param max_size 单个日志文件最大大小（字节），0表示不限制
     * @param max_files 最大日志文件数量
     */
    void setLogFile(std::string_view const & filepath, std::size_t max_size = 0, int max_files = 5);

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
     * @param args 格式参数
     * @param location 源代码位置信息
     */
    template<typename... Args>
    void logf(LogLevel const & level, const char * const format_str, Args&&... args,
              SourceLocation const & location = {}) {
        if (!shouldLog(level)) return;
        
        try {
            // 使用简单的字符串格式化（C++17兼容）
            std::ostringstream oss;
            formatImpl(oss, format_str, std::forward<Args>(args)...);
            log(level, oss.str(), location);
        } catch (const std::exception& e) {
            // 格式化失败时记录错误
            std::ostringstream error_oss;
            error_oss << "Log format error: " << e.what();
            log(LogLevel::ERROR_LEVEL, error_oss.str(), location);
        }
    }
    
    /**
     * @brief 兼容旧接口的日志记录
     * @param level 日志级别
     * @param file 源文件名
     * @param line 行号
     * @param function 函数名
     * @param message 日志消息
     */
    [[deprecated("Use log() with SourceLocation instead")]]
    void log(LogLevel const & level, const char* file, int line,
             const char* function, std::string_view const & message);
    
    /**
     * @brief 刷新所有待处理的日志
     */
    void flush();
    
    /**
     * @brief 等待所有日志处理完成
     * @param timeout 超时时间（毫秒），0表示无限等待
     * @return 是否在超时前完成
     */
    [[nodiscard]] bool waitForCompletion(std::chrono::milliseconds const & timeout = std::chrono::milliseconds::zero());
    
    /**
     * @brief 获取当前队列大小
     * @return 队列中待处理的日志数量
     */
    [[nodiscard]] std::size_t getQueueSize() const;
    
    /**
     * @brief 检查是否应该记录指定级别的日志
     * @param level 日志级别
     * @return 是否应该记录
     */
    [[nodiscard]] bool shouldLog(LogLevel const & level) const noexcept {
        return level >= m_log_level_.load(std::memory_order_relaxed);
    }

    /**
     * @brief 获取日志级别名称
     * @param level 日志级别
     * @return 级别名称
     */
    [[nodiscard]] static constexpr std::string_view getLevelName(LogLevel const & level) noexcept {
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

private:
    XLog();
    ~XLog();
    bool construct_();
    X_DISABLE_COPY_MOVE(XLog)
    
    // 格式化辅助函数
    template<typename T>
    inline static void formatImpl(std::ostringstream& oss, const char* const format, T&& value) {
        auto p{format};
        while (*p) {
            if (*p == '%' && *(p + 1) != '%') {
                oss << std::forward<T>(value);
                // 跳过格式说明符
                while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\0') { ++p; }
                if (*p) { oss << p; }
                return;
            } else if (*p == '%' && *(p + 1) == '%') {
                oss << '%';
                p += 2;
            } else {
                oss << *p++;
            }
        }
    }
    
    template<typename T, typename... Args>
    inline static void formatImpl(std::ostringstream& oss, const char * const format, T&& value, Args&&... args) {
        auto p{format};
        while (*p) {
            if (*p == '%' && *(p + 1) != '%') {
                oss << std::forward<T>(value);
                // 跳过格式说明符
                while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\0') { ++p; }
                if (*p) { formatImpl(oss, p, std::forward<Args>(args)...); }
                return;
            } else if (*p == '%' && *(p + 1) == '%') {
                oss << '%';
                p += 2;
            } else {
                oss << *p++;
            }
        }
    }
    
    inline static void formatImpl(std::ostringstream& oss, const char* const format) {
        oss << format;
    }

    // 异步日志处理
    void processLogQueue();
    void writeToConsole(const LogMessage& ) const;
    void writeToFile(const LogMessage& );
    [[nodiscard]] static std::string formatLogMessage(const LogMessage& ) ;

    // 文件轮转
    void rotateLogFile();
    [[nodiscard]] bool shouldRotateFile() const noexcept;
    [[nodiscard]] std::string getRotatedFileName(int ) const;
    
    // 崩溃处理
    static void setupCrashHandlers();
    static void removeCrashHandlers() noexcept;
    static void handleCrash(int );
    static void writeCrashLog(std::string_view const & );

#ifdef _WIN32
    static LONG WINAPI handleWindowsException(EXCEPTION_POINTERS* ex_info);
#endif

};

// 现代化的便利宏定义
#define XLOG_TRACE(msg) \
    do { \
        if (auto const _logger_ {XUtils::XLog::instance()}; \
            _logger_ && _logger_->shouldLog(XUtils::LogLevel::TRACE_LEVEL)) { \
            _logger_->log(XUtils::LogLevel::TRACE_LEVEL, msg, XUtils::SourceLocation::current(__FILE__, __FUNCTION__, __LINE__)); \
        } \
    } while(false)

#define XLOG_DEBUG(msg) \
    do { \
        if (auto const _logger_ {XUtils::XLog::instance()}; \
            _logger_ && _logger_->shouldLog(XUtils::LogLevel::DEBUG_LEVEL)) { \
            _logger_->log(XUtils::LogLevel::DEBUG_LEVEL, msg, XUtils::SourceLocation::current(__FILE__, __FUNCTION__, __LINE__)); \
        } \
    } while(false)

#define XLOG_INFO(msg) \
    do { \
        if (auto const _logger_ {XUtils::XLog::instance()}; \
            _logger_ && _logger_->shouldLog(XUtils::LogLevel::INFO_LEVEL)) { \
            _logger_->log(XUtils::LogLevel::INFO_LEVEL, msg, XUtils::SourceLocation::current(__FILE__, __FUNCTION__, __LINE__)); \
        } \
    } while(false)

#define XLOG_WARN(msg) \
    do { \
        if (auto const _logger_ {XUtils::XLog::instance()}; \
            _logger_ && _logger_->shouldLog(XUtils::LogLevel::WARN_LEVEL)) { \
            _logger_->log(XUtils::LogLevel::WARN_LEVEL, msg, XUtils::SourceLocation::current(__FILE__, __FUNCTION__, __LINE__)); \
        } \
    } while(false)

#define XLOG_ERROR(msg) \
    do { \
        if (auto const _logger_ {XUtils::XLog::instance()}; \
            _logger_ && _logger_->shouldLog(XUtils::LogLevel::ERROR_LEVEL)) { \
            _logger_->log(XUtils::LogLevel::ERROR_LEVEL, msg, XUtils::SourceLocation::current(__FILE__, __FUNCTION__, __LINE__)); \
        } \
    } while(false)

#define XLOG_FATAL(msg) \
    do { \
        if (auto const _logger_ {XUtils::XLog::instance()}; \
            _logger_ && _logger_->shouldLog(XUtils::LogLevel::FATAL_LEVEL)) { \
            _logger_->log(XUtils::LogLevel::FATAL_LEVEL, msg, XUtils::SourceLocation::current(__FILE__, __FUNCTION__, __LINE__)); \
            _logger_->flush(); \
        } \
    } while(false)

// 格式化日志宏
#define XLOGF_TRACE(fmt, ...) \
    do { \
        if (auto const _logger_ {XUtils::XLog::instance()}; \
            _logger_ && _logger_->shouldLog(XUtils::LogLevel::TRACE_LEVEL)) { \
            _logger_->logf(XUtils::LogLevel::TRACE_LEVEL, fmt, __VA_ARGS__); \
        } \
    } while(false)

#define XLOGF_DEBUG(fmt, ...) \
    do { \
        if (auto const _logger_ {XUtils::XLog::instance()}; \
            _logger_ && _logger_->shouldLog(XUtils::LogLevel::DEBUG_LEVEL)) { \
            _logger_->logf(XUtils::LogLevel::DEBUG_LEVEL, fmt, __VA_ARGS__); \
        } \
    } while(false)

#define XLOGF_INFO(fmt, ...) \
    do { \
        if (auto const _logger_ {XUtils::XLog::instance()}; \
            _logger_ && _logger_->shouldLog(XUtils::LogLevel::INFO_LEVEL)) { \
            _logger_->logf(XUtils::LogLevel::INFO_LEVEL, fmt, __VA_ARGS__); \
        } \
    } while(false)

#define XLOGF_WARN(fmt, ...) \
    do { \
        if (auto const _logger_ {XUtils::XLog::instance()}; \
            _logger_ && _logger_->shouldLog(XUtils::LogLevel::WARN_LEVEL)) { \
            _logger_->logf(XUtils::LogLevel::WARN_LEVEL, fmt, __VA_ARGS__); \
        } \
    } while(false)

#define XLOGF_ERROR(fmt, ...) \
    do { \
        if (auto const _logger_ {XUtils::XLog::instance()}; \
            _logger_ && _logger_->shouldLog(XUtils::LogLevel::ERROR_LEVEL)) { \
            _logger_->logf(XUtils::LogLevel::ERROR_LEVEL, fmt, __VA_ARGS__); \
        } \
    } while(false)

#define XLOGF_FATAL(fmt, ...) \
    do { \
        if (auto const _logger_ {XUtils::XLog::instance()}; \
            _logger_ && _logger_->shouldLog(XUtils::LogLevel::FATAL_LEVEL)) { \
            _logger_->logf(XUtils::LogLevel::FATAL_LEVEL, fmt, __VA_ARGS__); \
        } \
    } while(false)

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
