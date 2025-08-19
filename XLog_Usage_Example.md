# XLog 使用示例

XLog 是一个功能强大的线程安全异步日志系统，支持控制台和文件输出，并提供崩溃诊断功能。

## 特性

- ✅ **线程安全**：支持多线程并发日志记录
- ✅ **异步处理**：不阻塞主线程，高性能日志记录
- ✅ **多输出支持**：控制台、文件或同时输出
- ✅ **日志级别**：TRACE、DEBUG、INFO、WARN、ERROR、FATAL
- ✅ **彩色输出**：控制台彩色日志显示
- ✅ **文件轮转**：自动日志文件轮转管理
- ✅ **崩溃诊断**：自动捕获崩溃并生成堆栈跟踪
- ✅ **格式化日志**：支持printf风格的格式化
- ✅ **自定义崩溃处理**：可扩展的崩溃处理机制

## 基本使用

### 1. 初始化日志系统

```cpp
#include <XLog/xlog.hpp>
using namespace XUtils;

int main() {
    // 获取日志实例（单例模式）
    auto logger = XLog::UniqueConstruction();
    if (!logger) {
        std::cerr << "Failed to initialize logger!" << std::endl;
        return 1;
    }
    
    // 配置日志系统
    logger->setLogLevel(LogLevel::DEBUG);           // 设置最低日志级别
    logger->setOutput(LogOutput::BOTH);             // 同时输出到控制台和文件
    logger->setLogFile("app.log", 1024*1024, 5);   // 设置日志文件，1MB轮转，最多5个文件
    logger->setColorOutput(true);                   // 启用彩色输出
    logger->enableCrashDiagnostics(true);           // 启用崩溃诊断
    
    return 0;
}
```

### 2. 基本日志记录

```cpp
// 使用便利宏记录日志
XLOG_TRACE("详细跟踪信息");
XLOG_DEBUG("调试信息");
XLOG_INFO("一般信息");
XLOG_WARN("警告信息");
XLOG_ERROR("错误信息");
XLOG_FATAL("致命错误");

// 格式化日志
XLOGF_INFO("用户 %s 登录成功，ID: %d", username.c_str(), user_id);
XLOGF_ERROR("连接失败，错误码: %d, 重试次数: %d", error_code, retry_count);
```

### 3. 多线程日志记录

```cpp
#include <thread>
#include <vector>

void workerThread(int thread_id) {
    for (int i = 0; i < 100; ++i) {
        XLOGF_INFO("线程 %d 处理任务 %d", thread_id, i);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

int main() {
    auto logger = XLog::UniqueConstruction();
    
    // 创建多个工作线程
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(workerThread, i);
    }
    
    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    // 确保所有日志都被处理
    logger->flush();
    
    return 0;
}
```

## 高级功能

### 1. 自定义崩溃处理器

```cpp
class MyCrashHandler : public ICrashHandler {
public:
    void onCrash(const std::string& crash_info) override {
        // 发送崩溃报告到服务器
        sendCrashReport(crash_info);
        
        // 保存用户数据
        saveUserData();
        
        // 显示友好的错误对话框
        showErrorDialog("程序遇到了意外错误，即将退出");
    }
    
private:
    void sendCrashReport(const std::string& info) {
        // 实现崩溃报告发送逻辑
    }
    
    void saveUserData() {
        // 实现数据保存逻辑
    }
    
    void showErrorDialog(const std::string& message) {
        // 实现错误对话框显示
    }
};

int main() {
    auto logger = XLog::UniqueConstruction();
    
    // 设置自定义崩溃处理器
    auto crash_handler = std::make_shared<MyCrashHandler>();
    logger->setCrashHandler(crash_handler);
    
    return 0;
}
```

### 2. 性能监控和队列管理

```cpp
void monitorLogQueue() {
    auto logger = XLog::instance();
    
    while (running) {
        size_t queue_size = logger->getQueueSize();
        if (queue_size > 1000) {
            XLOG_WARN("日志队列积压严重，当前大小: " + std::to_string(queue_size));
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main() {
    auto logger = XLog::UniqueConstruction();
    
    // 设置队列大小限制
    logger->setAsyncQueueSize(5000);
    
    // 启动监控线程
    std::thread monitor_thread(monitorLogQueue);
    
    // 应用程序逻辑...
    
    // 优雅关闭
    logger->waitForCompletion(5000);  // 等待5秒让所有日志处理完成
    
    monitor_thread.join();
    return 0;
}
```

### 3. 条件日志记录

```cpp
void processData(const std::vector<int>& data) {
    XLOGF_INFO("开始处理 %zu 条数据", data.size());
    
    for (size_t i = 0; i < data.size(); ++i) {
        // 只在DEBUG级别记录详细处理信息
        XLOGF_DEBUG("处理第 %zu 条数据: %d", i, data[i]);
        
        if (data[i] < 0) {
            XLOGF_WARN("发现负数数据: %d at index %zu", data[i], i);
        }
        
        // 处理数据的逻辑...
    }
    
    XLOG_INFO("数据处理完成");
}
```

## 配置选项

### 日志级别设置

```cpp
// 只记录INFO及以上级别的日志
logger->setLogLevel(LogLevel::INFO);

// 在调试模式下记录所有日志
#ifdef DEBUG
    logger->setLogLevel(LogLevel::TRACE);
#else
    logger->setLogLevel(LogLevel::WARN);
#endif
```

### 输出方式配置

```cpp
// 只输出到控制台
logger->setOutput(LogOutput::CONSOLE);

// 只输出到文件
logger->setOutput(LogOutput::FILE);

// 同时输出到控制台和文件
logger->setOutput(LogOutput::BOTH);
```

### 文件轮转配置

```cpp
// 设置日志文件，10MB轮转，保留10个历史文件
logger->setLogFile("myapp.log", 10 * 1024 * 1024, 10);

// 不限制文件大小（不轮转）
logger->setLogFile("myapp.log", 0, 1);
```

## 日志格式

默认日志格式：
```
[2024-01-15 14:30:25.123] [INFO] [thread_id] filename.cpp:42 function_name() - Log message
```

格式说明：
- `[2024-01-15 14:30:25.123]`：时间戳（精确到毫秒）
- `[INFO]`：日志级别
- `[thread_id]`：线程ID
- `filename.cpp:42`：源文件名和行号
- `function_name()`：函数名
- `Log message`：日志消息内容

## 性能考虑

1. **异步处理**：日志记录操作不会阻塞主线程
2. **队列管理**：可配置队列大小防止内存过度使用
3. **级别过滤**：在记录前就过滤不需要的日志级别
4. **批量处理**：内部批量处理日志消息提高效率

## 最佳实践

1. **合理设置日志级别**：生产环境使用INFO或WARN级别
2. **避免频繁刷新**：只在关键时刻调用flush()
3. **监控队列大小**：防止日志积压影响性能
4. **使用格式化日志**：提高日志的可读性
5. **设置崩溃处理器**：及时响应程序崩溃
6. **定期清理日志文件**：避免磁盘空间不足

## 注意事项

1. 日志系统使用单例模式，全局只有一个实例
2. 程序退出时会自动等待所有日志处理完成
3. 崩溃处理器中不应该抛出异常
4. 文件轮转在写入时检查，可能有轻微延迟
5. 在多线程环境下，日志顺序可能与实际执行顺序略有差异

## 编译要求

- C++17 或更高版本
- 支持 std::filesystem
- 支持 std::thread 和相关同步原语
- Linux/macOS: 需要 execinfo.h（用于堆栈跟踪）
- Windows: 需要 dbghelp.lib（用于堆栈跟踪） 