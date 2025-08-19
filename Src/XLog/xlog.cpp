#include <xlog.hpp>
#include <iostream>
#include <filesystem>
#include <algorithm>

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
    s_instance_ = nullptr;
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
        return false;
    }
}

void XLog::setLogLevel(LogLevel const & level) noexcept {
    m_log_level_.store(level, std::memory_order_relaxed);
}

LogLevel XLog::getLogLevel() const noexcept {
    return m_log_level_.load(std::memory_order_relaxed);
}

void XLog::setOutput(LogOutput output) noexcept {
    m_output_.store(output, std::memory_order_relaxed);
}

void XLog::setLogFile(std::string_view filepath, std::size_t max_size, int max_files) {
    std::unique_lock lock(m_config_mutex_);
    
    m_log_file_path_ = filepath;
    m_max_file_size_.store(max_size, std::memory_order_relaxed);
    m_max_files_.store(max_files, std::memory_order_relaxed);
    
    // 重新打开文件流
    std::lock_guard file_lock(m_file_mutex_);
    m_file_stream_.reset();
    m_current_file_size_.store(0, std::memory_order_relaxed);
}

void XLog::setColorOutput(bool enable) noexcept {
    m_color_output_.store(enable, std::memory_order_relaxed);
}

void XLog::setAsyncQueueSize(std::size_t size) noexcept {
    m_max_queue_size_.store(size, std::memory_order_relaxed);
}

void XLog::enableCrashDiagnostics(bool enable) {
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

void XLog::log(LogLevel level, std::string_view message, SourceLocation location) {
    if (!shouldLog(level)) return;
    
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

// 兼容旧接口
void XLog::log(LogLevel level, const char* file, int line, 
               const char* function, std::string_view message) {
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
    
    if (timeout == std::chrono::milliseconds::zero()) {
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
    const auto now = std::chrono::system_clock::now();
    const auto time_t = std::chrono::system_clock::to_time_t(now);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

std::string XLog::getCurrentThreadId() {
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    return oss.str();
}

std::string XLog::getStackTrace(int skip_frames) {
    std::string result;
    
#ifdef _WIN32
    // Windows 堆栈跟踪
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();
    
    CONTEXT context{};
    context.ContextFlags = CONTEXT_FULL;
    RtlCaptureContext(&context);
    
    SymInitialize(process, nullptr, TRUE);
    
    DWORD image;
    STACKFRAME64 stackframe{};
    
#ifdef _M_IX86
    image = IMAGE_FILE_MACHINE_I386;
    stackframe.AddrPC.Offset = context.Eip;
    stackframe.AddrPC.Mode = AddrModeFlat;
    stackframe.AddrFrame.Offset = context.Ebp;
    stackframe.AddrFrame.Mode = AddrModeFlat;
    stackframe.AddrStack.Offset = context.Esp;
    stackframe.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
    image = IMAGE_FILE_MACHINE_AMD64;
    stackframe.AddrPC.Offset = context.Rip;
    stackframe.AddrPC.Mode = AddrModeFlat;
    stackframe.AddrFrame.Offset = context.Rsp;
    stackframe.AddrFrame.Mode = AddrModeFlat;
    stackframe.AddrStack.Offset = context.Rsp;
    stackframe.AddrStack.Mode = AddrModeFlat;
#endif
    
    int frame_count = 0;
    constexpr int max_frames = 64;
    std::array<char, 256> symbol_buffer{};
    auto* symbol = reinterpret_cast<SYMBOL_INFO*>(symbol_buffer.data());
    symbol->MaxNameLen = symbol_buffer.size() - sizeof(SYMBOL_INFO);
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    
    while (StackWalk64(image, process, thread, &stackframe, &context,
                      nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr) 
           && frame_count < max_frames) {
        
        if (frame_count >= skip_frames) {
            DWORD64 address = stackframe.AddrPC.Offset;
            if (SymFromAddr(process, address, nullptr, symbol)) {
                std::ostringstream oss;
                oss << "  #" << (frame_count - skip_frames) << ": " << symbol->Name 
                    << " [0x" << std::hex << address << "]";
                result += oss.str() + '\n';
            } else {
                std::ostringstream oss;
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
    constexpr int max_frames = 64;
    std::array<void*, max_frames> buffer{};
    
    const int nptrs = backtrace(buffer.data(), max_frames);
    if (nptrs > skip_frames) {
        char** strings = backtrace_symbols(buffer.data(), nptrs);
        if (strings) {
            for (int i = skip_frames; i < nptrs; ++i) {
                std::ostringstream oss;
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
    const std::string formatted = formatLogMessage(msg);
    
    if (m_color_output_.load(std::memory_order_relaxed)) {
        // 添加颜色代码
        std::string_view color_code;
        constexpr std::string_view reset_code = "\033[0m";
        
        switch (msg.level) {
            case LogLevel::TRACE: color_code = "\033[37m"; break;  // 白色
            case LogLevel::DEBUG: color_code = "\033[36m"; break;  // 青色
            case LogLevel::INFO:  color_code = "\033[32m"; break;  // 绿色
            case LogLevel::WARN:  color_code = "\033[33m"; break;  // 黄色
            case LogLevel::ERROR: color_code = "\033[31m"; break;  // 红色
            case LogLevel::FATAL: color_code = "\033[35m"; break;  // 紫色
        }
        
        if (msg.level >= LogLevel::ERROR) {
            std::cerr << color_code << formatted << reset_code << '\n';
        } else {
            std::cout << color_code << formatted << reset_code << '\n';
        }
    } else {
        if (msg.level >= LogLevel::ERROR) {
            std::cerr << formatted << '\n';
        } else {
            std::cout << formatted << '\n';
        }
    }
}

void XLog::writeToFile(const LogMessage& msg) {
    std::lock_guard lock(m_file_mutex_);
    
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
    const std::string formatted = formatLogMessage(msg);
    *m_file_stream_ << formatted << '\n';
    m_file_stream_->flush();
    
    // 更新文件大小
    m_current_file_size_.fetch_add(formatted.length() + 1, std::memory_order_relaxed);
}

std::string XLog::formatLogMessage(const LogMessage& msg) const {
    std::ostringstream oss;
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
        const int max_files = m_max_files_.load(std::memory_order_relaxed);
        
        // 删除最旧的文件
        const std::string oldest_file = getRotatedFileName(max_files - 1);
        if (std::filesystem::exists(oldest_file)) {
            std::filesystem::remove(oldest_file);
        }
        
        // 重命名现有文件
        for (int i = max_files - 2; i >= 0; --i) {
            const std::string old_name = (i == 0) ? m_log_file_path_ : getRotatedFileName(i);
            const std::string new_name = getRotatedFileName(i + 1);
            
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
    const std::size_t max_size = m_max_file_size_.load(std::memory_order_relaxed);
    return max_size > 0 && m_current_file_size_.load(std::memory_order_relaxed) >= max_size;
}

std::string XLog::getRotatedFileName(int index) const {
    const std::filesystem::path path(m_log_file_path_);
    const std::string stem = path.stem().string();
    const std::string extension = path.extension().string();
    const std::string parent = path.parent_path().string();
    
    std::ostringstream oss;
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

void XLog::handleCrash(int signal) {
    std::ostringstream oss;
    oss << "Application crashed with signal: " << signal << "\nStack trace:\n" << getStackTrace(2);
    const std::string crash_info = oss.str();
    
    writeCrashLog(crash_info);
    
    if (s_instance_ && s_instance_->m_crash_handler_) {
        try {
            s_instance_->m_crash_handler_->onCrash(crash_info);
        } catch (...) {
            // 忽略崩溃处理器中的异常
        }
    }
    
    // 恢复默认处理器并重新触发信号
    std::signal(signal, SIG_DFL);
    std::raise(signal);
}

void XLog::writeCrashLog(std::string_view crash_info) {
    try {
        std::ostringstream filename_oss;
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
LONG WINAPI XLog::handleWindowsException(EXCEPTION_POINTERS* ex_info) {
    std::ostringstream oss;
    oss << "Windows exception occurred: 0x" << std::hex 
        << ex_info->ExceptionRecord->ExceptionCode << std::dec 
        << "\nStack trace:\n" << getStackTrace(0);
    const std::string crash_info = oss.str();
    
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
