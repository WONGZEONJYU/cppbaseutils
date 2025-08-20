#include "xlog.hpp"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <csignal>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#else
#include <execinfo.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
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
        // 设置默认日志文件路径
        m_log_file_path_ = "application.log";
        
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

void XLog::setLogFile(std::string_view const & filepath, std::size_t const max_size, int const max_files) {
    std::unique_lock lock(m_config_mutex_);
    
    m_log_file_path_ = filepath;
    m_max_file_size_.store(max_size, std::memory_order_relaxed);
    m_max_files_.store(max_files, std::memory_order_relaxed);
    
    // 重新打开文件流
    std::lock_guard file_lock(m_file_mutex_);
    m_file_stream_.reset();
    m_current_file_size_.store(0, std::memory_order_relaxed);
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

void XLog::setCrashHandler(CrashHandlerPtr handler) {
    std::unique_lock lock(m_config_mutex_);
    m_crash_handler_ = std::move(handler);
}

void XLog::log(LogLevel const & level, std::string_view const & message, SourceLocation const & location) {
    if (!shouldLog(level)) {
        return;
    }

    try {
        // 创建日志消息
        LogMessage log_msg(
            level,
            getCurrentTimestamp(),
            getCurrentThreadId(),
            location.m_fileName ? std::filesystem::path(location.m_fileName).filename().string() : "unknown",
            location.m_line,
            location.m_functionName ? location.m_functionName : "unknown",
            std::string(message)
        );

        // 添加到队列
        {
            std::unique_lock lock(m_queue_mutex_);
            
            // 检查队列大小限制
            const auto max_size{m_max_queue_size_.load(std::memory_order_relaxed)};
            if (max_size > 0 && m_log_queue_.size() >= max_size) {
                // 队列满时丢弃最旧的消息
                m_log_queue_.pop();
            }

            m_log_queue_.push(std::move(log_msg));
        }
        
        // 通知处理线程
        m_queue_cv_.notify_one();
        
    } catch (const std::exception& e) {
        // 如果日志系统本身出错，直接输出到stderr
        std::cerr << "XLog error: " << e.what() << '\n';
    }
}

// 兼容旧接口
void XLog::log(LogLevel const & level, const char * const file, int const line,
               const char * const function, std::string_view const & message) {
    if (!shouldLog(level)) return;

    try {
        // 创建日志消息
        LogMessage log_msg(
            level,
            getCurrentTimestamp(),
            getCurrentThreadId(),
            file ? std::filesystem::path(file).filename().string() : "unknown",
            static_cast<std::uint32_t>(line),
            function ? function : "unknown",
            std::string(message)
        );
        
        // 添加到队列
        {
            std::unique_lock lock(m_queue_mutex_);
            
            // 检查队列大小限制
            const auto max_size = m_max_queue_size_.load(std::memory_order_relaxed);
            if (max_size > 0 && m_log_queue_.size() >= max_size) {
                // 队列满时丢弃最旧的消息
                m_log_queue_.pop();
            }
            
            m_log_queue_.push(std::move(log_msg));
        }
        
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
    std::lock_guard file_lock(m_file_mutex_);
    if (m_file_stream_ && m_file_stream_->is_open()) {
        m_file_stream_->flush();
    }
    std::cout.flush();
    std::cerr.flush();
}

bool XLog::waitForCompletion(std::chrono::milliseconds timeout) {
    std::unique_lock lock(m_queue_mutex_);

    if ( timeout == std::chrono::milliseconds::zero() ) {
        m_queue_cv_.wait(lock, [this] { return m_log_queue_.empty(); });
        return true;
    } else {
        return m_queue_cv_.wait_for(lock, timeout,
                                   [this] { return m_log_queue_.empty(); });
    }
}

std::size_t XLog::getQueueSize() const {
    std::shared_lock lock(m_queue_mutex_);
    return m_log_queue_.size();
}

std::string XLog::getCurrentTimestamp() {
    using namespace std::chrono;
    const auto now{system_clock::now()};
    const auto time_t{system_clock::to_time_t(now)};
    const auto ms{duration_cast<milliseconds>(now.time_since_epoch()) % 1000};

    return (std::ostringstream{}
        << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count()).str();
}

std::string XLog::getCurrentThreadId() {
    return (std::ostringstream{} << std::this_thread::get_id()).str() ;
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
    stackFrame_.AddrPC.Offset = context.Eip;
    stackFrame_.AddrPC.Mode = AddrModeFlat;
    stackFrame_.AddrFrame.Offset = context.Ebp;
    stackFrame_.AddrFrame.Mode = AddrModeFlat;
    stackFrame_.AddrStack.Offset = context.Esp;
    stackFrame_.AddrStack.Mode = AddrModeFlat;
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
        m_queue_cv_.wait(lock, [this] {
            return !m_log_queue_.empty() || m_shutdown_requested_.load();
        });

        // 处理队列中的所有消息
        while (!m_log_queue_.empty()) {
            LogMessage msg = std::move(m_log_queue_.front());
            m_log_queue_.pop();
            lock.unlock();
            
            // 处理日志消息
            const auto output = m_output_.load(std::memory_order_relaxed);
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
        m_file_stream_ = std::make_unique<std::ofstream>(
            m_log_file_path_, std::ios::app);
        
        if (!m_file_stream_->is_open()) {
            std::cerr << "Failed to open log file: " << m_log_file_path_ << '\n';
            return;
        }
        
        // 获取当前文件大小
        try {
            if (std::filesystem::exists(m_log_file_path_)) {
                m_current_file_size_.store(
                    std::filesystem::file_size(m_log_file_path_), 
                    std::memory_order_relaxed);
            }
        } catch (const std::exception&) {
            m_current_file_size_.store(0, std::memory_order_relaxed);
        }
    }

    // 写入日志
    auto const formatted{formatLogMessage(msg)};
    *m_file_stream_ << formatted << '\n';
    m_file_stream_->flush();

    // 更新文件大小
    m_current_file_size_.fetch_add(formatted.length() + 1, std::memory_order_relaxed);
}

std::string XLog::formatLogMessage(const LogMessage& msg)  {
    std::ostringstream oss{};
    oss << "[" << msg.timestamp << "] "
        << "[" << getLevelName(msg.level) << "] "
        << "[" << msg.thread_id << "] "
        << msg.file << ":" << msg.line << " "
        << msg.function << "() - "
        << msg.message;
    return oss.str();
}

void XLog::rotateLogFile() {
    if (m_file_stream_ && m_file_stream_->is_open()) {
        m_file_stream_->close();
        m_file_stream_.reset();
    }
    
    try {
        auto const max_files{m_max_files_.load(std::memory_order_relaxed)};

        // 删除最旧的文件
        auto const oldest_file{getRotatedFileName(max_files - 1)};
        if (std::filesystem::exists(oldest_file)) {
            std::filesystem::remove(oldest_file);
        }

        // 重命名现有文件
        for (int i {max_files - 2}; i >= 0; --i) {
            auto const old_name{ !i ? m_log_file_path_ : getRotatedFileName(i)},
                    new_name{getRotatedFileName(i + 1)};

            if (std::filesystem::exists(old_name)) {
                std::filesystem::rename(old_name, new_name);
            }
        }
        
        m_current_file_size_.store(0, std::memory_order_relaxed);
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to rotate log file: " << e.what() << '\n';
    }
}

bool XLog::shouldRotateFile() const noexcept {
    auto const max_size{m_max_file_size_.load(std::memory_order_relaxed)};
    return max_size > 0 && m_current_file_size_.load(std::memory_order_relaxed) >= max_size;
}

std::string XLog::getRotatedFileName(int const index) const {
    const std::filesystem::path path(m_log_file_path_);
    auto const stem{path.stem().string()}
        ,extension{path.extension().string()}
        ,parent{path.parent_path().string()};

    std::ostringstream oss{};
    if (!parent.empty()) {
        oss << parent << "/";
    }
    oss << stem << "." << index << extension;
    return oss.str();
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

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
