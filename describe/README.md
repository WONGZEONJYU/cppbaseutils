# XUtils - C++基础工具库

## 概述

XUtils是一个现代化的C++基础工具库，提供线程池、日志系统、信号槽、对象管理等功能。库设计为**编译时不依赖Qt**，但支持在用户项目中**选择性启用Qt功能**。

## 特性

- 🚀 **无Qt依赖编译** - 库本身可以独立编译，不依赖Qt
- 🔧 **可选Qt功能** - 用户项目可以选择性启用Qt相关辅助函数
- 🧵 **线程池** - 高性能线程池实现
- 📝 **日志系统** - 灵活的日志记录功能
- 🔗 **信号槽** - 类型安全的信号槽系统
- 🏗️ **对象管理** - 智能指针和对象生命周期管理
- ⚡ **原子操作** - 跨平台原子操作封装
- 🛠️ **工具函数** - 常用工具函数集合

## 快速开始

### 编译库（无Qt依赖）

```bash
mkdir build
cd build
cmake ..  # 默认不依赖Qt
make
```

### 在用户项目中使用

#### 不使用Qt功能
```cpp
#include <XHelper/xhelper.hpp>

class MyClass : public xtd::XHelperClass<MyClass> {
    X_HELPER_CLASS
private:
    bool construct_() { return true; }
    MyClass() = default;
public:
    void doSomething() { /* 实现 */ }
};

auto obj = MyClass::CreateSharedPtr();
```

#### 使用Qt功能
```cpp
// 在CMakeLists.txt中：
find_package(Qt6 COMPONENTS Core REQUIRED)
target_link_libraries(your_target PRIVATE Qt6::Core)
// 无需手动定义宏，库会自动检测Qt！

// 在代码中：
auto qptr = MyClass::CreateQScopedPointer();
auto qshared = MyClass::CreateQSharedPointer();
```

## 模块说明

- **XAtomic** - 原子操作
- **XGlobal** - 全局类型和工具
- **XHelper** - 辅助类和工具函数
- **XLog** - 日志系统
- **XObject** - 对象管理
- **XSignal** - 信号槽系统
- **XThreadPool** - 线程池
- **XTools** - 工具函数

## 文档

- [Qt功能使用指南](README_QT_USAGE.md) - 如何在用户项目中启用Qt功能
- [使用示例](example_usage.md) - 详细的使用示例
- [重构说明](README_REFACTOR.md) - 重构历史记录

## 构建要求

- C++20 编译器
- CMake 3.20+
- 可选：Qt5/6（仅用户项目需要时）

## 许可证

[添加您的许可证信息]
