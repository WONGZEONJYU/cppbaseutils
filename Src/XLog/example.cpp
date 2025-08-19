/**
 * @file example.cpp
 * @brief XLog使用示例
 * 
 * 这个示例展示了XLog的基本使用方法
 * 编译命令：g++ -std=c++17 -I. example.cpp -o example
 */

#include "xlog.hpp"
#include <thread>
#include <chrono>

void simulateWork(const std::string& taskName, int duration) {
    XLOG_INFO("开始执行任务: " + taskName);
    
    for (int i = 0; i < duration; ++i) {
        XLOG_DEBUG("任务 " + taskName + " 进度: " + std::to_string(i + 1) + "/" + std::to_string(duration));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    XLOG_INFO("任务完成: " + taskName);
}

int main() {
    std::cout << "=== XLog 使用示例 ===" << std::endl;
    
    // 1. 初始化日志系统
    auto logger = XLog::UniqueConstruction();
    
    // 2. 配置日志系统
    logger->setLogLevel(LogLevel::DEBUG);
    logger->setOutputMode(OutputMode::BOTH);  // 同时输出到控制台和文件
    logger->setLogFile("example.log");
    
    // 3. 基本日志记录
    XLOG_INFO("程序启动");
    XLOG_DEBUG("当前日志级别: DEBUG");
    
    // 4. 不同级别的日志
    XLOG_TRACE("这是跟踪信息（可能不会显示，因为级别是DEBUG）");
    XLOG_DEBUG("这是调试信息");
    XLOG_INFO("这是一般信息");
    XLOG_WARN("这是警告信息");
    XLOG_ERROR("这是错误信息");
    
    // 5. 模拟一些工作
    simulateWork("数据处理", 3);
    
    // 6. 多线程示例
    XLOG_INFO("启动多线程任务");
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back([i]() {
            std::string threadName = "Thread-" + std::to_string(i);
            simulateWork(threadName, 2);
        });
    }
    
    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    // 7. 程序结束
    XLOG_INFO("所有任务完成");
    logger->flush();  // 确保所有日志都被写入
    
    std::cout << "=== 示例程序结束，请查看 example.log 文件 ===" << std::endl;
    
    return 0;
} 