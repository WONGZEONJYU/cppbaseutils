#include <XLog/xlog.hpp>
#include <iostream>
#include <thread>
#include <vector>

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
    std::cout << "=== 测试基本日志功能 ===" << std::endl;
    const auto logger{XlogHandle()};
    if (!logger) {
        std::cerr << "Failed to create logger instance!" << std::endl;
        return;
    }

    // 配置日志系统
    logger->setLogLevel(LogLevel::TRACE_LEVEL);
    logger->setOutput(LogOutput::BOTH);
    logger->setLogFileConfig("test_basic", "test_logs", 1024, 7);  // 使用现代化配置方法
    logger->setColorOutput(true);

    // 测试各种日志级别
    XLOG_TRACE("这是一条TRACE日志");
    XLOG_DEBUG("这是一条DEBUG日志");
    XLOG_INFO("这是一条INFO日志");
    XLOG_WARN("这是一条WARN日志");
    XLOG_ERROR("这是一条ERROR日志");
    XLOG_FATAL("这是一条FATAL日志");

    // 刷新确保写入
    logger->flush();
    std::cout << "基本日志测试完成" << std::endl;
}

/**
 * @brief 测试格式化日志功能
 */
void testFormattedLogging() {
    std::cout << "\n=== Testing Formatted Logging ===\n";

    int count = 42;
    double value = 3.14159;
    const char* name = "XLog";
    
    // 测试所有格式化日志级别
    XLOGF_TRACE("TRACE: Processing %d items with value %.2f", count, value);
    XLOGF_DEBUG("DEBUG: %s system initialized with %d threads", name, count);
    XLOGF_INFO("INFO: Operation completed successfully in %.3f seconds", value);
    XLOGF_WARN("WARN: Memory usage is at %d%% capacity", 85);
    XLOGF_ERROR("ERROR: Failed to process %d/%d items", 38, count);
    XLOGF_FATAL("FATAL: Critical system failure - error code: %d",500);
    
    // 测试复杂格式化
    XLOGF_INFO("Complex format: string='%s', int=%d, float=%.2f, hex=0x%X", 
               name, count, value, 255);

    std::cout << "Formatted logging test completed.\n";
    XlogHandle()->flush();
}

/**
 * @brief 测试配置功能
 */
void testConfiguration() {
    std::cout << "\n=== Testing Configuration ===\n";
    
    auto const logger{XUtils::XlogHandle()};
    
    // 测试日志级别设置
    logger->setLogLevel(LogLevel::DEBUG_LEVEL);
    std::cout << "Current log level: " << static_cast<int>(logger->getLogLevel()) << "\n";
    
    // 测试文件输出设置
    logger->setLogFileConfig("test_config", "test_logs", 1, 1);  // 使用现代化配置方法
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

    constexpr auto num_threads {4},messages_per_thread {10};

    std::vector<std::thread> threads{};
    threads.reserve(num_threads);

    for (int t {}; t < num_threads; ++t) {
        threads.emplace_back([t] {
            for (int i {}; i < messages_per_thread; ++i) {
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

/**
 * @brief 性能测试 - 测试优化后宏的性能
 */
void testPerformance() {
    std::cout << "\n=== Testing Performance ===\n";
    
    constexpr int test_count = 1000;
    
    // 测试普通日志宏性能
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < test_count; ++i) {
        XLOG_INFO("Performance test message #" + std::to_string(i) + " - optimized macros should have better performance");
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Regular logging: " << test_count << " messages in " << duration.count() << " ms\n";
    
    // 测试格式化日志宏性能
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < test_count; ++i) {
        XLOGF_INFO("Formatted performance test #%d - %.2f%% complete", i, (i * 100.0) / test_count);
    }
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Formatted logging: " << test_count << " messages in " << duration.count() << " ms\n";
    std::cout << "Performance test completed.\n";
}

int main() {
    std::cout << "Starting comprehensive XLog test...\n";

    try {
        // 初始化日志实例
        auto const logger{XlogHandle()};
        if (!logger) {
            std::cout << "Failed to create logger instance\n";
            return -1;
        }

        std::cout << "Created logger instance successfully\n";

        // 设置基本配置
        logger->setLogLevel(LogLevel::TRACE_LEVEL);
        logger->setOutput(LogOutput::BOTH);
        logger->setColorOutput(true);

        // 运行各项测试
        testBasicLogging();
        testFormattedLogging();
        //testConfiguration();
        //testMultiThreadLogging();
        //testPerformance();

        // 测试崩溃处理器设置
        std::cout << "\n=== Testing Crash Handler ===\n";
        auto const crash_handler{makeShared<CustomCrashHandler>()};
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