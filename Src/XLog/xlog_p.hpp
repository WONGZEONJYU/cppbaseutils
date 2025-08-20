#ifndef XUTILS_XLOG_P_HPP
#define XUTILS_XLOG_P_HPP

#include <XLog/xlog.hpp>
#include <string>
#include <string_view>
#include <memory>
#include <atomic>
#include <fstream>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <queue>
#include <condition_variable>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XLogPrivate final: public XLogData {
public:
    X_DECLARE_PUBLIC(XLog)
    using CrashHandlerPtr = std::shared_ptr<ICrashHandler>;
private:
    // 配置参数
    std::atomic<LogLevel> m_log_level_ {LogLevel::INFO_LEVEL};
    std::atomic<LogOutput> m_output_ {LogOutput::BOTH};
    std::atomic_bool m_color_output_{true}
                    ,m_crash_diagnostics_{true};
    std::atomic_size_t m_max_queue_size_ {10000};

    // 文件相关
    std::string m_log_file_path_{}
                ,m_log_base_name_{"application"} // 基础文件名
                ,m_log_directory_{"logs"};      // 日志目录
    std::atomic_size_t m_max_file_size_{5 * 1024 * 1024}; // 默认5MB
    std::atomic_int m_max_files_{5}
                ,m_retention_days_{7}; // 默认保存7天
    std::unique_ptr<std::ofstream> m_file_stream_{};
    std::atomic_size_t m_current_file_size_{};
    std::string m_current_log_file_{}; // 当前正在使用的日志文件名

    // 异步处理
    std::queue<LogMessage> m_log_queue_{};
    mutable std::shared_mutex m_queue_mutex_{};
    std::condition_variable_any m_queue_cv_{};
    std::thread m_worker_thread_{};
    std::atomic_bool m_running_{}
    ,m_shutdown_requested_{};

    // 崩溃处理
    CrashHandlerPtr m_crash_handler_{};

    // 同步
    mutable std::shared_mutex m_config_mutex_{};
    mutable std::mutex m_file_mutex_{};

public:
    explicit XLogPrivate(XLog * );
    ~XLogPrivate() override = default;

    // 异步日志处理
    void processLogQueue();
    void writeToConsole(const LogMessage& ) const;
    void writeToFile(const LogMessage& );
    [[nodiscard]] static std::string formatLogMessage(const LogMessage& ) ;

    // 文件轮转
    void rotateLogFile();
    [[nodiscard]] bool shouldRotateFile() const noexcept;

    // 崩溃处理
    static void setupCrashHandlers();
    static void removeCrashHandlers() noexcept;
    static void handleCrash(int );
    static void writeCrashLog(std::string_view const & );

    // 文件管理
    void initializeLogFile();
    [[nodiscard]] std::string generateLogFileName() const;
    [[nodiscard]] std::string findLatestLogFile() const;
    [[nodiscard]] bool isLogFileFromToday(std::string_view const & filename) const;
    [[nodiscard]] std::string getTodayDateString() const;
    [[nodiscard]] std::string getLogFilePattern() const;
    void ensureLogDirectory() const;

#ifdef _WIN32
    static LONG WINAPI handleWindowsException(EXCEPTION_POINTERS* ex_info);
#endif
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
