# XLog 格式化宏功能

## 概述

XLog 现在支持 printf 风格的格式化宏，提供了类似 `printf` 的格式化功能，同时保持了原有的类型安全和异步性能特性。

## 新增的格式化宏

### 基本格式化宏

- `XLOGF_TRACE(fmt, ...)` - TRACE 级别的格式化日志
- `XLOGF_DEBUG(fmt, ...)` - DEBUG 级别的格式化日志  
- `XLOGF_INFO(fmt, ...)`  - INFO 级别的格式化日志
- `XLOGF_WARN(fmt, ...)`  - WARN 级别的格式化日志
- `XLOGF_ERROR(fmt, ...)` - ERROR 级别的格式化日志
- `XLOGF_FATAL(fmt, ...)` - FATAL 级别的格式化日志（自动刷新）

### 使用示例

```cpp
#include "XLog/xlog.hpp"

int main() {
    // 初始化日志系统
    auto logger = XUtils::XLog::UniqueConstruction();
    logger->setLogLevel(XUtils::LogLevel::TRACE_LEVEL);
    
    // 无参数使用
    XLOGF_INFO("Simple message");
    
    // 单参数使用
    int value = 42;
    XLOGF_INFO("Value: %d", value);
    
    // 多参数使用
    std::string name = "Alice";
    int age = 25;
    double score = 95.5;
    XLOGF_INFO("User: %s, Age: %d, Score: %.1f", name.c_str(), age, score);
    
    // 复杂格式化
    XLOGF_DEBUG("Hex: 0x%x, Binary: %d, Float: %.3f", 255, 255, 3.14159);
    
    return 0;
}
```

## 技术实现

### C++20 实现

对于支持 C++20 的编译器，使用 `__VA_OPT__` 特性来处理可变参数：

```cpp
#define XLOGF_INFO(fmt, ...) XLOGF_IMPL(XUtils::LogLevel::INFO_LEVEL, fmt, __VA_ARGS__)
```

### C++17 兼容性

对于 C++17 编译器，同样的宏定义可以工作，因为现代编译器对可变参数宏有良好的支持。

### 格式化引擎

内部使用 `std::snprintf` 进行格式化，支持所有标准的 printf 格式说明符：

- `%d`, `%i` - 十进制整数
- `%x`, `%X` - 十六进制整数
- `%o` - 八进制整数
- `%f`, `%F` - 浮点数
- `%e`, `%E` - 科学计数法
- `%g`, `%G` - 自动选择格式
- `%s` - 字符串
- `%c` - 字符
- `%p` - 指针
- `%%` - 字面量 %

### 性能特性

1. **零开销抽象** - 编译时优化，运行时性能与手写代码相同
2. **异步处理** - 格式化在后台线程进行，不阻塞主线程
3. **内存安全** - 自动处理缓冲区大小，防止溢出
4. **异常安全** - 格式化错误不会导致程序崩溃

### 与原有宏的对比

| 原有宏 | 新格式化宏 | 功能差异 |
|--------|------------|----------|
| `XLOG_INFO("message")` | `XLOGF_INFO("message")` | 无参数时功能相同 |
| 不支持 | `XLOGF_INFO("Value: %d", 42)` | 支持参数格式化 |
| 需要手动拼接 | `XLOGF_INFO("User: %s, Age: %d", name, age)` | 自动格式化 |

## 最佳实践

1. **选择合适的宏**：
   - 简单消息使用原有宏：`XLOG_INFO("Started")`
   - 需要参数时使用格式化宏：`XLOGF_INFO("User %s logged in", username)`

2. **格式安全**：
   - 确保格式字符串与参数类型匹配
   - 使用 `%s` 时确保传递 C 风格字符串（如 `str.c_str()`）

3. **性能考虑**：
   - 格式化宏有轻微的额外开销，但在异步模式下影响很小
   - 对于高频日志，考虑使用适当的日志级别过滤

## 编译器支持

- **C++20**: 完全支持，使用 `__VA_OPT__` 优化
- **C++17**: 完全支持，使用兼容性实现
- **GCC 8+**: 支持
- **Clang 10+**: 支持  
- **MSVC 2019+**: 支持

## 注意事项

1. **FATAL 宏**: `XLOGF_FATAL` 会自动调用 `flush()`，确保致命错误被立即写入
2. **线程安全**: 所有格式化宏都是线程安全的
3. **异常处理**: 格式化错误会被捕获并记录为错误日志，不会导致程序崩溃
4. **缓冲区管理**: 自动处理大消息的缓冲区分配 