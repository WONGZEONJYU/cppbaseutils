#include <XLog/xlog.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

using namespace XUtils;

/**
 * @brief 自定义崩溃处理器示例
 */
class CustomCrashHandler : public ICrashHandler {
public:
    void onCrash(std::string_view const & crash_info) override {
        std::cout << "\n=== Custom Crash Handler ===\n";
        std::cout << "Application is about to crash!\n";
        std::cout << "Crash info:\n" << crash_info << std::endl;
        
        // 这里可以添加自定义的崩溃处理逻辑
        // 比如发送崩溃报告、保存用户数据等
    }
};

/**
 * @brief 测试基本日志功能
 */
void testBasicLogging() {
    std::cout << "\n=== Testing Basic Logging ===\n";
    
    auto logger = XLog::instance();
    
    // 测试所有日志级别
    XLOG_TRACE("This is a TRACE message");
    XLOG_DEBUG("This is a DEBUG message");
    XLOG_INFO("This is an INFO message");
    XLOG_WARN("This is a WARN message");
    XLOG_ERROR("This is an ERROR message");
    XLOG_FATAL("This is a FATAL message");
    
    // 测试直接调用log方法
    logger->log(LogLevel::INFO_LEVEL, "Direct log call test");
    
    std::cout << "Basic logging test completed.\n";
}

/**
 * @brief 测试配置功能
 */
void testConfiguration() {
    std::cout << "\n=== Testing Configuration ===\n";
    
    auto logger = XLog::instance();
    
    // 测试日志级别设置
    logger->setLogLevel(LogLevel::DEBUG_LEVEL);
    std::cout << "Current log level: " << static_cast<int>(logger->getLogLevel()) << "\n";
    
    // 测试文件输出设置
    logger->setLogFile("test_log.txt", 1024 * 1024, 3);  // 1MB, 3个文件
    logger->setOutput(LogOutput::BOTH);
    
    // 测试彩色输出
    logger->setColorOutput(true);
    
    // 测试队列大小设置
    logger->setAsyncQueueSize(5000);
    
    std::cout << "Configuration test completed.\n";
}

/**
 * @brief 多线程日志测试
 */
void testMultiThreadLogging() {
    std::cout << "\n=== Testing Multi-threaded Logging ===\n";
    

    const int num_threads = 4;
    const int messages_per_thread = 10;
    
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([t]() {
            for (int i = 0; i < messages_per_thread; ++i) {
                XLOG_DEBUG("Thread " + std::to_string(t) + " processing item " + std::to_string(i));
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::cout << "Multi-threaded logging completed.\n";
}

int main() {
    std::cout << "Starting comprehensive XLog test...\n";
    
    try {
        // 初始化日志实例
        auto logger = XLog::UniqueConstruction();
        if (!logger) {
            std::cout << "Failed to create logger instance\n";
            return 1;
        }
        
        std::cout << "Created logger instance successfully\n";
        
        // 设置基本配置
        logger->setLogLevel(LogLevel::TRACE_LEVEL);
        logger->setOutput(LogOutput::BOTH);
        logger->setColorOutput(true);
        
        // 运行各项测试
        testBasicLogging();
        testConfiguration();
        testMultiThreadLogging();
        
        // 测试崩溃处理器设置
        std::cout << "\n=== Testing Crash Handler ===\n";
        auto crash_handler = std::make_shared<CustomCrashHandler>();
        logger->setCrashHandler(crash_handler);
        logger->enableCrashDiagnostics(true);
        std::cout << "Crash handler setup completed.\n";
        
        // 等待所有日志处理完成
        logger->flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::cout << "\n=== All Tests Completed Successfully ===\n";
        
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << '\n';
        return 1;
    }
    
    return 0;
} 