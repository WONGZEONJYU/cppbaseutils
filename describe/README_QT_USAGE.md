# Qt功能使用指南

## 概述

XUtils库设计为编译时不依赖Qt，但支持在用户项目中启用Qt相关功能。

## 库编译（无Qt依赖）

默认情况下，XUtils库编译时不依赖Qt：

```bash
mkdir build
cd build
cmake ..  # 默认DISABLE_QT=ON
make
```

## 用户项目中使用Qt功能

### 方法一：CMake项目（推荐）

如果您的项目使用CMake，**无需手动定义任何宏**，库会自动检测Qt：

```cmake
# 查找XUtils库
find_package(XUtils REQUIRED)

# 查找Qt（如果需要Qt功能）
find_package(Qt6 COMPONENTS Core REQUIRED)  # 或 Qt5

# 链接库
target_link_libraries(your_target 
    PRIVATE XUtils::XUtils
    PRIVATE Qt6::Core  # 链接Qt库后，XUtils会自动启用Qt功能
)

# 无需手动定义HAS_QT宏！
```

### 方法二：手动编译

```bash
# 编译您的项目时链接Qt库，XUtils会自动检测
g++ -I/path/to/xutils/include your_file.cpp -L/path/to/xutils/lib -lXUtils -lQt6Core
```

### 方法三：强制控制Qt功能

如果需要手动控制Qt功能：

```cmake
# 强制启用Qt功能（即使没有链接Qt库）
target_compile_definitions(your_target PRIVATE XUTILS_FORCE_QT)

# 强制禁用Qt功能（即使链接了Qt库）
target_compile_definitions(your_target PRIVATE XUTILS_DISABLE_QT)
```

## 可用的Qt功能

启用Qt功能后，您可以使用以下辅助函数：

### 1. Qt智能指针创建
```cpp
#include <XHelper/xhelper.hpp>

class MyClass : public xtd::XHelperClass<MyClass> {
    X_HELPER_CLASS
private:
    bool construct_() { /* 初始化代码 */ return true; }
    MyClass() = default;
public:
    void doSomething() { /* 实现 */ }
};

// 使用Qt智能指针
auto qptr = MyClass::CreateQScopedPointer();
auto qshared = MyClass::CreateQSharedPointer();
```

### 2. Qt枚举处理
```cpp
#include <XHelper/xhelper.hpp>

enum class MyEnum { Value1, Value2 };

// 获取枚举类型和值名称
QString enumInfo = MyClass::getEnumTypeAndValueName(MyEnum::Value1);
```

### 3. Qt对象查找
```cpp
#include <XHelper/xhelper.hpp>

// 通过名称查找子对象
QWidget* child = MyClass::findChildByName<QWidget>(parent, "childName");
```

### 4. Qt信号槽连接
```cpp
#include <XHelper/xhelper.hpp>

// 连接信号槽
auto connection = MyClass::ConnectHelper(sender, &Sender::signal, receiver, &Receiver::slot);
```

## 自动检测机制

XUtils使用智能的Qt检测机制：

1. **自动检测**：当用户项目链接Qt库时，XUtils会自动检测并启用Qt功能
2. **检测优先级**：
   - 用户显式定义（`XUTILS_FORCE_QT` 或 `XUTILS_DISABLE_QT`）
   - Qt宏检测（`QT_VERSION`、`QT_CORE_LIB`等）
   - Qt版本宏（`QT_VERSION_MAJOR`）
   - Qt模块宏（`QT_WIDGETS_LIB`、`QT_GUI_LIB`等）

## 注意事项

1. **库本身不依赖Qt**：XUtils库可以独立编译和分发
2. **自动检测Qt**：用户链接Qt库后，XUtils会自动启用Qt功能
3. **条件编译**：所有Qt功能都通过自动检测的`HAS_QT`宏控制
4. **向后兼容**：不链接Qt库时，库完全可用，只是没有Qt相关辅助函数
5. **手动控制**：可以通过`XUTILS_FORCE_QT`或`XUTILS_DISABLE_QT`强制控制

## 示例项目

参考`example_usage.md`中的完整使用示例。
