#include "xlog.hpp"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <regex>
#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#else
#include <csignal>
#include <execinfo.h>
#include <unistd.h>
#include <sys/types.h>
#endif

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

XLog::XLog() {
    s_instance_ = this;
}

XLog::~XLog() {
    if (m_running_.load()) {
        m_shutdown_requested_.store(true);
        m_queue_cv_.notify_all();

        if (m_worker_thread_.joinable()) {
            m_worker_thread_.join();
        }
    }

    removeCrashHandlers();
    s_instance_ = {};
}

bool XLog::construct_() {
    try {
        // 确保日志目录存在
        ensureLogDirectory();
        
        // 初始化日志文件（智能续写或创建新文件）
        initializeLogFile();
        
        // 清理过期的日志文件
        cleanupOldLogFiles();
        
        // 启动异步处理线程
        m_running_.store(true);
        m_worker_thread_ = std::thread(&XLog::processLogQueue, this);

        // 设置崩溃处理器
        if (m_crash_diagnostics_.load()) {
            setupCrashHandlers();
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize XLog: " << e.what() << '\n';
        return {};
    }
}

void XLog::setLogLevel(LogLevel const & level) noexcept {
    m_log_level_.store(level, std::memory_order_relaxed);
}

LogLevel XLog::getLogLevel() const noexcept {
    return m_log_level_.load(std::memory_order_relaxed);
}

void XLog::setOutput(LogOutput const & output) noexcept {
    m_output_.store(output, std::memory_order_relaxed);
}

void XLog::setLogFileConfig(std::string_view const & base_name, 
                           std::string_view const & directory,
                           std::size_t const max_size_mb, 
                           int const retention_days) {
    std::unique_lock lock(m_config_mutex_);
    
    m_log_base_name_ = base_name;
    m_log_directory_ = directory;
    m_max_file_size_.store(max_size_mb * 1024 * 1024, std::memory_order_relaxed);
    m_retention_days_.store(retention_days, std::memory_order_relaxed);
    
    // 重新初始化文件
    std::unique_lock file_lock(m_file_mutex_);
    m_file_stream_.reset();
    m_current_file_size_.store(0, std::memory_order_relaxed);
    
    // 确保目录存在并初始化新的日志文件
    ensureLogDirectory();
    initializeLogFile();
    cleanupOldLogFiles();
}

std::string XLog::getCurrentLogFile() const {
    std::shared_lock lock(m_config_mutex_);
    return m_current_log_file_;
}

void XLog::cleanupOldLogFiles() {
    try {
        if (!std::filesystem::exists(m_log_directory_)) {
            return;
        }
        
        auto const retention_days = m_retention_days_.load(std::memory_order_relaxed);
        if (retention_days <= 0) {
            return; // 不限制保存天数
        }
        
        auto const cutoff_time = std::chrono::system_clock::now() - 
                                std::chrono::hours(24 * retention_days);
        
        // 获取日志文件模式
        auto const pattern = getLogFilePattern();
        std::regex const log_regex(pattern);
        
        for (auto const& entry : std::filesystem::directory_iterator(m_log_directory_)) {
            if (!entry.is_regular_file()) continue;
            
            auto const filename = entry.path().filename().string();
            if (!std::regex_match(filename, log_regex)) continue;
            
            try {
                auto const file_time = std::filesystem::last_write_time(entry.path());
                auto const sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    file_time - std::filesystem::file_time_type::clock::now() + 
                    std::chrono::system_clock::now());
                
                if (sctp < cutoff_time) {
                    std::filesystem::remove(entry.path());
                }
            } catch (const std::exception& e) {
                std::cerr << "Failed to check/remove old log file " << filename 
                         << ": " << e.what() << '\n';
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to cleanup old log files: " << e.what() << '\n';
    }
}

void XLog::setColorOutput(bool const enable) noexcept {
    m_color_output_.store(enable, std::memory_order_relaxed);
}

void XLog::setAsyncQueueSize(std::size_t const size) noexcept {
    m_max_queue_size_.store(size, std::memory_order_relaxed);
}

void XLog::enableCrashDiagnostics(bool const enable) {
    m_crash_diagnostics_.store(enable, std::memory_order_relaxed);
    
    if (enable) {
        setupCrashHandlers();
    } else {
        removeCrashHandlers();
    }
}

void XLog::setCrashHandler(CrashHandlerPtr && handler) {
    std::unique_lock lock(m_config_mutex_);
    m_crash_handler_ = std::move(handler);
}

void XLog::log(LogLevel const & level, std::string_view const & message, SourceLocation const & location) {
    if (!shouldLog(level)) {
        return;
    }

    try {
        // 创建日志消息
        LogMessage log_msg {
            level,
            getCurrentTimestamp(),
            getCurrentThreadId(),
            location.m_fileName ? std::filesystem::path(location.m_fileName).filename().string() : "unknown",
            location.m_line,
            location.m_functionName ? location.m_functionName : "unknown",
            std::string(message)
        };

        std::unique_lock lock(m_queue_mutex_);

        // 检查队列大小限制
        if (const auto max_size{m_max_queue_size_.load(std::memory_order_relaxed)}
            ; max_size > 0 && m_log_queue_.size() >= max_size) {
            // 队列满时丢弃最旧的消息
            m_log_queue_.pop();
        }
        // 添加到队列
        m_log_queue_.push(std::move(log_msg));

        // 通知处理线程
        m_queue_cv_.notify_one();
        
    } catch (const std::exception& e) {
        // 如果日志系统本身出错，直接输出到stderr
        std::cerr << "XLog error: " << e.what() << '\n';
    }
}

void XLog::flush() {
    // 等待队列清空
    std::unique_lock lock(m_queue_mutex_);
    m_queue_cv_.wait(lock, [this] { return m_log_queue_.empty(); });
    
    // 强制刷新文件流
    std::unique_lock file_lock(m_file_mutex_);
    if (m_file_stream_ && m_file_stream_->is_open()) {
        m_file_stream_->flush();
    }
    std::cout.flush();
    std::cerr.flush();
}

bool XLog::waitForCompletion(std::chrono::milliseconds const & timeout) {
    std::unique_lock lock(m_queue_mutex_);

    if ( std::chrono::milliseconds::zero() == timeout ) {
        m_queue_cv_.wait(lock, [this]{ return m_log_queue_.empty(); });
        return true;
    } else {
        return m_queue_cv_.wait_for(lock, timeout,
                                   [this]{ return m_log_queue_.empty(); });
    }
}

std::size_t XLog::getQueueSize() const {
    std::shared_lock lock(m_queue_mutex_);
    return m_log_queue_.size();
}

std::string XLog::getCurrentTimestamp() {
    using namespace std::chrono;
    
    // 获取当前时间点和毫秒精度
    const auto now{system_clock::now()};
    const auto time_t_value{system_clock::to_time_t(now)};
    const auto ms{duration_cast<milliseconds>(now.time_since_epoch()) % 1000};
    
    // C++17 RAII线程安全时间格式化器
    class SafeTimeFormatter {
        std::tm tm_buffer_{};
        bool valid_{};
    public:
        explicit SafeTimeFormatter(std::time_t const & time) {
            // 优先尝试线程安全的本地时间函数
#ifdef _WIN32
            valid_ = !localtime_s(&tm_buffer_, &time);
            if (!valid_) {
                // Fallback to UTC
                valid_ = !gmtime_s(&tm_buffer_, &time);
            }
#else
            // POSIX系统：使用线程安全版本
            valid_ = (localtime_r(&time, &tm_buffer_) != nullptr);
            if (!valid_) {
                // Fallback to UTC
                valid_ = (gmtime_r(&time, &tm_buffer_) != nullptr);
            }
#endif
        }
        // 禁用拷贝和移动，确保线程安全
        X_DISABLE_COPY_MOVE(SafeTimeFormatter)

        [[nodiscard]] bool is_valid() const noexcept { return valid_; }
        [[nodiscard]] const std::tm* get() const noexcept { 
            return valid_ ? &tm_buffer_ : nullptr; 
        }
    };
    
    // 使用RAII格式化器
    SafeTimeFormatter const formatter{time_t_value};

    if (!formatter.is_valid()) {
        // 如果时间格式化失败，返回固定的错误时间戳
        return "1970-01-01 00:00:00.000";
    }

    // 使用现代C++17流式语法格式化时间
    return (std::ostringstream{} << std::put_time(formatter.get(), "%Y-%m-%d %H:%M:%S")
                                << '.' << std::setfill('0') << std::setw(3) << ms.count()).str();
}

std::string XLog::getCurrentThreadId() {
    return ( std::ostringstream{} << std::this_thread::get_id() ).str() ;
}

std::string XLog::getStackTrace(int const skip_frames) {
    std::string result{};

#ifdef _WIN32
    // Windows 堆栈跟踪
    auto const process{GetCurrentProcess()},
                thread{GetCurrentThread()};
    
    CONTEXT context{};
    context.ContextFlags = CONTEXT_FULL;
    RtlCaptureContext(&context);
    
    SymInitialize(process, nullptr, TRUE);
    
    DWORD image{};
    STACKFRAME64 stackFrame{};
    
#ifdef _M_IX86
    image = IMAGE_FILE_MACHINE_I386;
    stackFrame.AddrPC.Offset = context.Eip;
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = context.Ebp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = context.Esp;
    stackFrame.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
    image = IMAGE_FILE_MACHINE_AMD64;
    stackFrame.AddrPC.Offset = context.Rip;
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = context.Rsp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = context.Rsp;
    stackFrame.AddrStack.Mode = AddrModeFlat;
#endif

    constexpr auto max_frames{64};

    std::array<char, 256> symbol_buffer{};

    auto const symbol{reinterpret_cast<SYMBOL_INFO *>(symbol_buffer.data())};
    symbol->MaxNameLen = static_cast<decltype(symbol->MaxNameLen)>(symbol_buffer.size() - sizeof(SYMBOL_INFO));
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    int frame_count{};
    while (StackWalk64(image, process, thread, &stackFrame, &context,
                       nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr)
           && frame_count < max_frames) {
        
        if (frame_count >= skip_frames) {
            DWORD64 address = stackFrame.AddrPC.Offset;
            if (SymFromAddr(process, address, nullptr, symbol)) {
                std::ostringstream oss{};
                oss << "  #" << (frame_count - skip_frames) << ": " << symbol->Name 
                    << " [0x" << std::hex << address << "]";
                result += oss.str() + '\n';
            } else {
                std::ostringstream oss{};
                oss << "  #" << (frame_count - skip_frames) << ": <unknown> [0x" 
                    << std::hex << address << "]";
                result += oss.str() + '\n';
            }
        }
        frame_count++;
    }
    
    SymCleanup(process);

#else
    // Unix/Linux 堆栈跟踪
    constexpr auto max_frames {64};
    std::array<void*, max_frames> buffer{};

    if (auto const nptrs{backtrace(buffer.data(), max_frames)}
        ;nptrs > skip_frames)
    {
        if (auto const strings{backtrace_symbols(buffer.data(), nptrs)}) {
            for (auto i {skip_frames}; i < nptrs; ++i) {
                std::ostringstream oss{};
                oss << "  #" << (i - skip_frames) << ": " << strings[i];
                result += oss.str() + '\n';
            }
            free(strings);
        }
    }
#endif

    return result.empty() ? "Stack trace not available\n" : result;
}

void XLog::processLogQueue() {

    while (m_running_.load() || !m_log_queue_.empty()) {
        std::unique_lock lock(m_queue_mutex_);
        
        // 等待新消息或停止信号
        m_queue_cv_.wait(lock, [this]{
            return !m_log_queue_.empty() || m_shutdown_requested_.load();
        });

        // 处理队列中的所有消息
        while (!m_log_queue_.empty()) {
            auto const msg{std::move(m_log_queue_.front())};
            m_log_queue_.pop();
            lock.unlock();
            
            // 处理日志消息
            const auto output {m_output_.load(std::memory_order_relaxed)};
            if ((output & LogOutput::CONSOLE) != LogOutput{}) {
                writeToConsole(msg);
            }
            if ((output & LogOutput::FILE) != LogOutput{}) {
                writeToFile(msg);
            }
            
            lock.lock();
        }
        
        // 通知等待队列清空的线程
        m_queue_cv_.notify_all();
        
        if (m_shutdown_requested_.load()) {
            break;
        }
    }
}

void XLog::writeToConsole(const LogMessage& msg) const {
    auto const formatted{formatLogMessage(msg)};

    if (m_color_output_.load(std::memory_order_relaxed)) {
        // 添加颜色代码
        std::string_view color_code{};

        switch (msg.level) {
            case LogLevel::TRACE_LEVEL: color_code = "\033[37m"; break;  // 白色
            case LogLevel::DEBUG_LEVEL: color_code = "\033[36m"; break;  // 青色
            case LogLevel::INFO_LEVEL:  color_code = "\033[32m"; break;  // 绿色
            case LogLevel::WARN_LEVEL:  color_code = "\033[33m"; break;  // 黄色
            case LogLevel::ERROR_LEVEL: color_code = "\033[31m"; break;  // 红色
            case LogLevel::FATAL_LEVEL: color_code = "\033[35m"; break;  // 紫色
        }

        constexpr std::string_view reset_code {"\033[0m"};

        if (msg.level >= LogLevel::ERROR_LEVEL) {
            std::cerr << color_code << formatted << reset_code << '\n';
        } else {
            std::cout << color_code << formatted << reset_code << '\n';
        }
    } else {
        if (msg.level >= LogLevel::ERROR_LEVEL) {
            std::cerr << formatted << '\n';
        } else {
            std::cout << formatted << '\n';
        }
    }
}

void XLog::writeToFile(const LogMessage& msg) {
    std::unique_lock lock(m_file_mutex_);

    // 检查是否需要轮转文件
    if (shouldRotateFile()) {
        rotateLogFile();
    }

    // 打开文件流（如果需要）
    if (!m_file_stream_ || !m_file_stream_->is_open()) {

        m_file_stream_ = makeUnique<std::ofstream>(m_current_log_file_, std::ios::app);

        // 检查智能指针是否创建成功
        if (!m_file_stream_) {
            std::cerr << "Failed to create file stream for: " << m_current_log_file_ << '\n';
            return;
        }

        if (!m_file_stream_->is_open()) {
            std::cerr << "Failed to open log file: " << m_current_log_file_ << '\n';
            return;
        }

        // 获取当前文件大小
        try {
            if (std::filesystem::exists(m_current_log_file_)) {
                m_current_file_size_.store(
                        std::filesystem::file_size(m_current_log_file_)
                       ,std::memory_order_relaxed
               );
            }
        } catch (const std::exception &) {
            m_current_file_size_.store(0, std::memory_order_relaxed);
        }
    }

    // 写入日志前再次检查文件流的有效性
    if (!m_file_stream_ || !m_file_stream_->is_open()) {
        std::cerr << "File stream is not available for writing\n";
        return;
    }

    // 写入日志
    auto const formatted{formatLogMessage(msg)};
    *m_file_stream_ << formatted << '\n';
    m_file_stream_->flush();

    // 更新文件大小
    m_current_file_size_.fetch_add(formatted.length() + 1, std::memory_order_relaxed);
}

std::string XLog::formatLogMessage(const LogMessage& msg)  {
    return (
        std::ostringstream {} << "[" << msg.timestamp << "] "
                             << "[" << getLevelName(msg.level) << "] "
                             << "[" << msg.thread_id << "] "
                             << msg.file << ":" << msg.line << " "
                             << msg.function << "() - "
                             << msg.message).str();
}

void XLog::rotateLogFile() {

    if (m_file_stream_ && m_file_stream_->is_open()) {
        m_file_stream_->close();
        m_file_stream_.reset();
    }

    try {
        // 生成新的日志文件名
        m_current_log_file_ = generateLogFileName();
        m_log_file_path_ = m_current_log_file_;
        m_current_file_size_.store(0, std::memory_order_relaxed);
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to rotate log file: " << e.what() << '\n';
    }
}

bool XLog::shouldRotateFile() const noexcept {
    auto const max_size{m_max_file_size_.load(std::memory_order_relaxed)};
    return max_size > 0 && m_current_file_size_.load(std::memory_order_relaxed) >= max_size;
}



void XLog::setupCrashHandlers() {
#ifdef _WIN32
    SetUnhandledExceptionFilter(handleWindowsException);
#else
    std::signal(SIGSEGV, handleCrash);
    std::signal(SIGABRT, handleCrash);
    std::signal(SIGFPE, handleCrash);
    std::signal(SIGILL, handleCrash);
    std::signal(SIGTERM, handleCrash);
#endif
}

void XLog::removeCrashHandlers() noexcept {
#ifdef _WIN32
    SetUnhandledExceptionFilter(nullptr);
#else
    std::signal(SIGSEGV, SIG_DFL);
    std::signal(SIGABRT, SIG_DFL);
    std::signal(SIGFPE, SIG_DFL);
    std::signal(SIGILL, SIG_DFL);
    std::signal(SIGTERM, SIG_DFL);
#endif
}

void XLog::handleCrash(int const signal) {
    std::ostringstream oss{};
    oss << "Application crashed with signal: " << signal << "\nStack trace:\n" << getStackTrace(2);
    auto const crash_info{oss.str()};
    
    writeCrashLog(crash_info);

    if (s_instance_ && s_instance_->m_crash_handler_) {
        try {
            s_instance_->m_crash_handler_->onCrash(crash_info);
        } catch (...) {
            // 忽略崩溃处理器中的异常
        }
    }

#ifndef _WIN32
    // 恢复默认处理器并重新触发信号 (仅Unix/Linux)
    std::signal(signal, SIG_DFL);
    std::raise(signal);
#else
    // Windows下直接退出，因为这个函数在Windows上不应该被调用
    // Windows使用SEH异常处理，不使用POSIX信号
    std::exit(EXIT_FAILURE);
#endif
}

void XLog::writeCrashLog(std::string_view const & crash_info) {
    try {
        std::ostringstream filename_oss{};
        filename_oss << "crash_" << getCurrentTimestamp() << ".log";
        std::ofstream crash_file(filename_oss.str());
        if (crash_file.is_open()) {
            crash_file << crash_info << '\n';
        }

        std::cerr << "CRASH DETECTED:\n" << crash_info << '\n';
        
    } catch (...) {
        // 如果写入文件失败，至少输出到stderr
        std::cerr << "CRASH DETECTED:\n" << crash_info << '\n';
    }
}

#ifdef _WIN32
LONG WINAPI XLog::handleWindowsException(EXCEPTION_POINTERS* const ex_info) {
    std::ostringstream oss{};
    oss << "Windows exception occurred: 0x" << std::hex 
        << ex_info->ExceptionRecord->ExceptionCode << std::dec 
        << "\nStack trace:\n" << getStackTrace(0);
    auto const crash_info {oss.str()};
    
    writeCrashLog(crash_info);
    
    if (s_instance_ && s_instance_->m_crash_handler_) {
        try {
            s_instance_->m_crash_handler_->onCrash(crash_info);
        } catch (...) {
            // 忽略崩溃处理器中的异常
        }
    }

    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

void XLog::initializeLogFile() {
    try {
        // 查找今天最新的日志文件
        auto const latest_file = findLatestLogFile();
        
        if (!latest_file.empty()) {
            // 检查文件大小是否超出限制
            auto const file_size = std::filesystem::file_size(latest_file);
            auto const max_size = m_max_file_size_.load(std::memory_order_relaxed);
            
            if (max_size == 0 || file_size < max_size) {
                // 文件未满，继续使用
                m_current_log_file_ = latest_file;
                m_log_file_path_ = latest_file;
                m_current_file_size_.store(file_size, std::memory_order_relaxed);
                return;
            }
        }
        
        // 需要创建新文件
        m_current_log_file_ = generateLogFileName();
        m_log_file_path_ = m_current_log_file_;
        m_current_file_size_.store(0, std::memory_order_relaxed);
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize log file: " << e.what() << '\n';
        // 回退到简单的文件名
        m_current_log_file_ = m_log_directory_ + "/application.log";
        m_log_file_path_ = m_current_log_file_;
        m_current_file_size_.store(0, std::memory_order_relaxed);
    }
}

std::string XLog::generateLogFileName() const {
    auto const today = getTodayDateString();
    int sequence = 1;
    
    std::string filename;
    do {
        std::ostringstream oss;
        oss << m_log_directory_ << "/" << m_log_base_name_ 
            << "_" << today << "_" << std::setfill('0') << std::setw(3) << sequence << ".log";
        filename = oss.str();
        sequence++;
    } while (std::filesystem::exists(filename));
    
    return filename;
}

std::string XLog::findLatestLogFile() const {
    try {
        if (!std::filesystem::exists(m_log_directory_)) {
            return {};
        }
        
        auto const today = getTodayDateString();
        std::string latest_file;
        int max_sequence = 0;
        
        // 获取日志文件模式
        auto const pattern = getLogFilePattern();
        std::regex const log_regex(pattern);
        
        for (auto const& entry : std::filesystem::directory_iterator(m_log_directory_)) {
            if (!entry.is_regular_file()) continue;
            
            auto const filename = entry.path().filename().string();
            std::smatch match;
            
            if (std::regex_match(filename, match, log_regex)) {
                auto const file_date = match[2].str();
                auto const sequence_str = match[3].str();
                
                // 只考虑今天的文件
                if (file_date == today) {
                    int const sequence = std::stoi(sequence_str);
                    if (sequence > max_sequence) {
                        max_sequence = sequence;
                        latest_file = entry.path().string();
                    }
                }
            }
        }
        
        return latest_file;
    } catch (const std::exception& e) {
        std::cerr << "Failed to find latest log file: " << e.what() << '\n';
        return {};
    }
}

bool XLog::isLogFileFromToday(std::string_view const & filename) const {
    auto const today = getTodayDateString();
    return filename.find(today) != std::string::npos;
}

std::string XLog::getTodayDateString() const {
    using namespace std::chrono;
    
    auto const now = system_clock::now();
    auto const time_t_value = system_clock::to_time_t(now);
    
    std::tm tm_buffer{};
    bool valid = false;
    
#ifdef _WIN32
    valid = !localtime_s(&tm_buffer, &time_t_value);
#else
    valid = (localtime_r(&time_t_value, &tm_buffer) != nullptr);
#endif
    
    if (!valid) {
        return "1970-01-01";
    }
    
    std::ostringstream oss;
    oss << std::put_time(&tm_buffer, "%Y-%m-%d");
    return oss.str();
}

std::string XLog::getLogFilePattern() const {
    // 匹配格式：basename_YYYY-MM-DD_NNN.log
    // 例如：application_2024-01-15_001.log
    std::ostringstream oss;
    oss << "(" << m_log_base_name_ << ")_(\\d{4}-\\d{2}-\\d{2})_(\\d{3})\\.log";
    return oss.str();
}

void XLog::ensureLogDirectory() const {
    try {
        if (!std::filesystem::exists(m_log_directory_)) {
            std::filesystem::create_directories(m_log_directory_);
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to create log directory " << m_log_directory_ 
                 << ": " << e.what() << '\n';
    }
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
