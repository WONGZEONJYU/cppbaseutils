# XLog - 现代化C++日志系统

XLog是一个高性能、线程安全的C++17日志库，提供异步日志记录、多种输出方式和丰富的配置选项。

## 特性

- ✅ **多级别日志**：支持TRACE、DEBUG、INFO、WARN、ERROR、FATAL六个级别
- ✅ **异步处理**：高性能异步日志队列，不阻塞主线程
- ✅ **线程安全**：完全线程安全，支持多线程并发写入
- ✅ **多种输出**：同时支持控制台彩色输出和文件记录
- ✅ **源码位置**：自动记录文件名、行号、函数名
- ✅ **文件轮转**：支持按大小和时间的日志文件轮转
- ✅ **崩溃处理**：集成崩溃处理和诊断功能
- ✅ **配置灵活**：运行时动态配置日志级别和输出方式

## 快速开始

### 1. 基本使用

```cpp
#include "xlog.hpp"

int main() {
    // 初始化日志系统
    auto logger = XLog::UniqueConstruction();
    
    // 使用普通日志宏
    XLOG_INFO("程序启动");
    XLOG_DEBUG("调试信息: " + std::to_string(42));
    XLOG_WARN("警告信息");
    XLOG_ERROR("错误信息");
    
    // 使用格式化日志宏
    int count = 100;
    double progress = 85.5;
    XLOGF_INFO("处理进度: %d/%d (%.1f%%)", 85, count, progress);
    XLOGF_DEBUG("用户 %s 登录成功，ID: %d", "admin", 1001);
    XLOGF_ERROR("连接失败，错误代码: %d, 重试 %d 次", 500, 3);
    
    return 0;
}
```

### 2. 配置日志系统

```cpp
#include "xlog.hpp"

int main() {
    auto logger = XLog::UniqueConstruction();
    
    // 设置日志级别
    logger->setLogLevel(LogLevel::DEBUG_LEVEL);
    
    // 设置输出方式
    logger->setOutput(LogOutput::BOTH); // 同时输出到控制台和文件
    
    // 启用彩色输出
    logger->setColorOutput(true);
    
    // 设置日志文件 (文件名, 最大大小, 最大文件数)
    logger->setLogFile("myapp.log", 10 * 1024 * 1024, 5);
    
    // 设置异步队列大小
    logger->setAsyncQueueSize(10000);
    
    XLOGF_INFO("配置完成，当前日志级别: %d", static_cast<int>(logger->getLogLevel()));
    
    return 0;
}
```

### 3. 多线程使用

```cpp
#include "xlog.hpp"
#include <thread>
#include <vector>

void workerThread(int threadId) {
    for (int i = 0; i < 100; ++i) {
        XLOGF_DEBUG("线程 %d 处理任务 %d", threadId, i);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

int main() {
    auto logger = XLog::UniqueConstruction();
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(workerThread, i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    return 0;
}
```

### 4. 格式化日志详细示例

```cpp
#include "xlog.hpp"

void demonstrateFormattedLogging() {
    auto logger = XLog::UniqueConstruction();
    
    // 基本格式化
    int userId = 1001;
    std::string username = "admin";
    XLOGF_INFO("用户登录: ID=%d, 用户名=%s", userId, username.c_str());
    
    // 数值格式化
    double temperature = 23.456;
    int humidity = 65;
    XLOGF_DEBUG("环境数据: 温度=%.1f°C, 湿度=%d%%", temperature, humidity);
    
    // 十六进制和八进制
    int errorCode = 255;
    XLOGF_ERROR("错误代码: 十进制=%d, 十六进制=0x%X, 八进制=%o", errorCode, errorCode, errorCode);
    
    // 字符串格式化
    const char* operation = "数据库连接";
    int retryCount = 3;
    XLOGF_WARN("%s失败，将在%d秒后重试第%d次", operation, 5, retryCount);
    
    // 布尔值（通过条件表达式）
    bool isConnected = false;
    XLOGF_INFO("连接状态: %s", isConnected ? "已连接" : "未连接");
    
    // 性能测量
    auto start = std::chrono::high_resolution_clock::now();
    // ... 执行一些操作 ...
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    XLOGF_INFO("操作完成，耗时: %ld 毫秒", duration.count());
}
```

## API参考

### 日志宏

#### 普通日志宏

| 宏 | 级别 | 用途 |
|---|---|---|
| `XLOG_TRACE(msg)` | TRACE | 详细跟踪信息 |
| `XLOG_DEBUG(msg)` | DEBUG | 调试信息 |
| `XLOG_INFO(msg)` | INFO | 一般信息 |
| `XLOG_WARN(msg)` | WARN | 警告信息 |
| `XLOG_ERROR(msg)` | ERROR | 错误信息 |
| `XLOG_FATAL(msg)` | FATAL | 致命错误（自动flush） |

#### 格式化日志宏

| 宏 | 级别 | 用途 |
|---|---|---|
| `XLOGF_TRACE(fmt, ...)` | TRACE | 格式化详细跟踪信息 |
| `XLOGF_DEBUG(fmt, ...)` | DEBUG | 格式化调试信息 |
| `XLOGF_INFO(fmt, ...)` | INFO | 格式化一般信息 |
| `XLOGF_WARN(fmt, ...)` | WARN | 格式化警告信息 |
| `XLOGF_ERROR(fmt, ...)` | ERROR | 格式化错误信息 |
| `XLOGF_FATAL(fmt, ...)` | FATAL | 格式化致命错误（自动flush） |

### 配置方法

```cpp
class XLog {
public:
    // 获取单例实例（需要先调用UniqueConstruction）
    static std::shared_ptr<XLog> instance();
    
    // 创建单例实例
    static std::shared_ptr<XLog> UniqueConstruction();
    
    // 设置日志级别
    void setLogLevel(LogLevel level);
    
    // 获取当前日志级别
    LogLevel getLogLevel() const;
    
    // 检查是否应该记录指定级别的日志
    bool shouldLog(LogLevel level) const;
    
    // 设置输出模式
    void setOutput(LogOutput output);
    
    // 设置彩色输出
    void setColorOutput(bool enable);
    
    // 设置日志文件（文件名，最大大小，最大文件数）
    void setLogFile(std::string_view filename, size_t maxSize = 10*1024*1024, size_t maxFiles = 5);
    
    // 设置异步队列大小
    void setAsyncQueueSize(size_t size);
    
    // 设置崩溃处理器
    void setCrashHandler(std::shared_ptr<ICrashHandler> handler);
    
    // 启用/禁用崩溃诊断
    void enableCrashDiagnostics(bool enable);
    
    // 刷新日志缓冲区
    void flush();
    
    // 记录日志（通常不直接调用，使用宏）
    void log(LogLevel level, std::string_view message, SourceLocation location = SourceLocation::current());
};
```

### 枚举类型

```cpp
enum class LogLevel {
    TRACE_LEVEL = 0,
    DEBUG_LEVEL = 1,
    INFO_LEVEL = 2,
    WARN_LEVEL = 3,
    ERROR_LEVEL = 4,
    FATAL_LEVEL = 5
};

enum class LogOutput {
    CONSOLE_ONLY,  // 仅控制台输出
    FILE_ONLY,     // 仅文件输出
    BOTH          // 同时输出到控制台和文件
};
```

## 日志格式

默认日志格式：
```
[2025-08-19 23:50:08.479] [INFO] [0x2089ea0c0] main.cpp:15 main() - 这是一条信息日志
```

格式说明：
- `[2025-08-19 23:50:08.479]`：时间戳（精确到毫秒）
- `[INFO]`：日志级别
- `[0x2089ea0c0]`：线程ID
- `main.cpp:15`：源文件名和行号
- `main()`：函数名
- `这是一条信息日志`：日志消息

## 性能特性

- **异步处理**：日志写入操作不会阻塞业务线程
- **内存池**：使用内存池减少内存分配开销
- **批量写入**：支持批量写入提高I/O效率
- **线程安全**：使用无锁队列实现高并发性能
- **优化宏设计**：减少代码重复，提高编译效率和运行时性能
- **级别检查**：自动检查日志级别，避免不必要的字符串构造
- **FATAL自动刷新**：FATAL级别日志自动调用flush()确保立即写入

## 最佳实践

### 1. 初始化

```cpp
// 在程序开始时初始化
auto logger = XLog::UniqueConstruction();

// 配置日志系统
logger->setLogLevel(LogLevel::INFO_LEVEL);
logger->setOutput(LogOutput::BOTH);
logger->setColorOutput(true);
logger->setLogFile("app.log", 50*1024*1024, 10); // 50MB, 保留10个文件
```

### 2. 日志级别使用建议

- **TRACE**：非常详细的调试信息，通常只在开发阶段使用
- **DEBUG**：调试信息，用于问题排查
- **INFO**：一般信息，记录程序运行状态
- **WARN**：警告信息，程序可以继续运行但需要注意
- **ERROR**：错误信息，程序遇到错误但可以恢复
- **FATAL**：致命错误，程序无法继续运行

### 3. 选择合适的日志宏

```cpp
// 简单字符串：使用普通宏
XLOG_INFO("程序启动完成");
XLOG_ERROR("连接失败");

// 字符串连接：使用普通宏
XLOG_INFO("用户登录: " + username);
XLOG_DEBUG("处理文件: " + filename);

// 多个变量或复杂格式：使用格式化宏（推荐）
XLOGF_INFO("用户 %s (ID: %d) 登录成功", username.c_str(), userId);
XLOGF_DEBUG("处理进度: %d/%d (%.1f%%)", current, total, percentage);

// 避免：不必要的字符串转换
// 不好：XLOG_DEBUG("数量: " + std::to_string(count));
// 好：  XLOGF_DEBUG("数量: %d", count);
```

### 4. 条件日志

```cpp
// 对于复杂的日志消息，可以先检查级别
if (logger->getLogLevel() <= LogLevel::DEBUG) {
    std::string complexMessage = buildComplexMessage();
    XLOG_DEBUG(complexMessage);
}
```

## 故障排除

### 常见问题

1. **段错误**：确保先调用`UniqueConstruction()`而不是直接使用`instance()`
2. **日志不输出**：检查日志级别设置是否正确
3. **文件无法创建**：检查文件路径权限和磁盘空间
4. **性能问题**：避免在高频循环中使用TRACE级别日志

### 调试模式

```cpp
// 启用详细日志用于调试
logger->setLogLevel(LogLevel::TRACE_LEVEL);
XLOG_TRACE("详细的调试信息");
XLOGF_TRACE("变量值: count=%d, status=%s", count, status.c_str());
```

## 编译要求

- C++17或更高版本
- CMake 3.10或更高版本
- 支持的编译器：GCC 7+, Clang 5+, MSVC 2017+

## 许可证

请参考项目根目录的LICENSE文件。 