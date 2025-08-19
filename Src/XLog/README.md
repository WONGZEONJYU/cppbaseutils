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
    
    // 使用宏记录日志
    XLOG_INFO("程序启动");
    XLOG_DEBUG("调试信息: " + std::to_string(42));
    XLOG_WARN("警告信息");
    XLOG_ERROR("错误信息");
    
    return 0;
}
```

### 2. 配置日志系统

```cpp
#include "xlog.hpp"

int main() {
    auto logger = XLog::UniqueConstruction();
    
    // 设置日志级别
    logger->setLogLevel(LogLevel::DEBUG);
    
    // 设置输出方式
    logger->setOutputMode(OutputMode::BOTH); // 同时输出到控制台和文件
    
    // 设置日志文件
    logger->setLogFile("myapp.log");
    
    // 启用文件轮转 (10MB, 保留5个文件)
    logger->enableFileRotation(10 * 1024 * 1024, 5);
    
    XLOG_INFO("配置完成");
    
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
        XLOG_DEBUG("线程 " + std::to_string(threadId) + " 处理任务 " + std::to_string(i));
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

## API参考

### 日志宏

| 宏 | 级别 | 用途 |
|---|---|---|
| `XLOG_TRACE(msg)` | TRACE | 详细跟踪信息 |
| `XLOG_DEBUG(msg)` | DEBUG | 调试信息 |
| `XLOG_INFO(msg)` | INFO | 一般信息 |
| `XLOG_WARN(msg)` | WARN | 警告信息 |
| `XLOG_ERROR(msg)` | ERROR | 错误信息 |
| `XLOG_FATAL(msg)` | FATAL | 致命错误 |

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
    
    // 设置输出模式
    void setOutputMode(OutputMode mode);
    
    // 设置日志文件路径
    void setLogFile(const std::string& filename);
    
    // 启用文件轮转
    void enableFileRotation(size_t maxSize, size_t maxFiles);
    
    // 禁用文件轮转
    void disableFileRotation();
    
    // 设置崩溃处理器
    void setCrashHandler(std::function<void()> handler);
    
    // 刷新日志缓冲区
    void flush();
};
```

### 枚举类型

```cpp
enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5
};

enum class OutputMode {
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

## 最佳实践

### 1. 初始化

```cpp
// 在程序开始时初始化
auto logger = XLog::UniqueConstruction();

// 配置日志系统
logger->setLogLevel(LogLevel::INFO);
logger->setOutputMode(OutputMode::BOTH);
logger->setLogFile("app.log");
```

### 2. 日志级别使用建议

- **TRACE**：非常详细的调试信息，通常只在开发阶段使用
- **DEBUG**：调试信息，用于问题排查
- **INFO**：一般信息，记录程序运行状态
- **WARN**：警告信息，程序可以继续运行但需要注意
- **ERROR**：错误信息，程序遇到错误但可以恢复
- **FATAL**：致命错误，程序无法继续运行

### 3. 字符串构建

```cpp
// 推荐：使用字符串连接
XLOG_INFO("用户登录: " + username);

// 推荐：使用std::to_string转换数字
XLOG_DEBUG("处理第 " + std::to_string(count) + " 个请求");

// 避免：复杂的字符串格式化（影响性能）
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
logger->setLogLevel(LogLevel::TRACE);
XLOG_TRACE("详细的调试信息");
```

## 编译要求

- C++17或更高版本
- CMake 3.10或更高版本
- 支持的编译器：GCC 7+, Clang 5+, MSVC 2017+

## 许可证

请参考项目根目录的LICENSE文件。 