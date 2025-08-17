# XUtils 重构说明

## 概述

本文档描述了 XUtils 库的重构过程和新的架构设计。

## 重构目标

1. **模块化设计**: 将原有的单一库拆分为多个功能模块
2. **更好的可维护性**: 清晰的目录结构和依赖关系
3. **现代化构建系统**: 使用 CMake 3.20+ 的现代特性
4. **跨平台支持**: 统一的构建和安装流程
5. **简化使用**: 提供统一的接口目标

## 新的目录结构

```
Src/
├── XAtomic/          # 原子操作模块
├── XGlobal/          # 全局类型和常量
├── XHelper/          # 辅助工具模块
├── XObject/          # 对象系统模块
├── XSignal/          # 信号槽系统（Unix/macOS）
├── XThreadPool/      # 线程池模块
├── XTools/           # 通用工具模块
└── cmake/            # 模块特定的 CMake 配置
```

## 模块说明

### XAtomic
- 原子操作封装
- 线程安全的数据结构
- 内存序控制

### XGlobal
- 全局类型定义
- 平台抽象层
- 常量定义

### XHelper
- 通用辅助函数
- Qt 集成工具
- 字符串处理

### XObject
- 对象基类
- 反射系统
- 属性系统

### XSignal (Unix/macOS)
- 信号处理
- 进程间通信
- 系统信号封装

### XThreadPool
- 线程池实现
- 任务调度
- 异步执行

### XTools
- 智能指针
- 通用工具类
- 调试工具

## 构建系统

### 现代化构建脚本

使用 `build_modern.sh` 脚本进行构建：

```bash
# 构建 Debug 版本的共享库
./build_modern.sh --debug --shared

# 构建 Release 版本的静态库
./build_modern.sh --release --static

# 同时构建所有版本
./build_modern.sh --multi --both

# 构建并安装
./build_modern.sh --install

# 运行测试
./build_modern.sh --test
```

### CMake 配置

项目使用现代的 CMake 配置：

- 最低版本要求：CMake 3.20
- C++ 标准：C++20
- 支持多配置构建
- 自动依赖管理

## 使用方式

### 简化链接方式 ✨

现在您可以使用简化的方式链接库，无需指定具体的静态或动态版本：

```cmake
# 在您的 CMakeLists.txt 中
find_package(XUtils REQUIRED)

# 简化的链接方式 - 自动选择静态或动态库
target_link_libraries(${PROJECT_NAME}
    XUtils
)
```

系统会根据构建配置自动选择：
- 如果构建了共享库，使用 `XUtils_shared`
- 如果构建了静态库，使用 `XUtils_static`
- 如果同时构建了两种库，优先使用共享库

### 传统链接方式

如果您需要明确指定库类型，仍然可以使用传统方式：

```cmake
# 明确指定共享库
target_link_libraries(${PROJECT_NAME}
    XUtils::XUtils_shared
)

# 明确指定静态库
target_link_libraries(${PROJECT_NAME}
    XUtils::XUtils_static
)
```

## 安装和分发

### 安装

```bash
# 构建并安装
./build_modern.sh --install

# 安装到自定义目录
./build_modern.sh --install --prefix /usr/local
```

### 在其他项目中使用

安装后，其他项目可以通过以下方式使用：

```cmake
find_package(XUtils REQUIRED)
target_link_libraries(your_target XUtils)
```

## 迁移指南

### 从旧版本迁移

1. **更新 CMakeLists.txt**:
   ```cmake
   # 旧方式
   target_link_libraries(${PROJECT_NAME} XUtils_shared)
   
   # 新方式
   target_link_libraries(${PROJECT_NAME} XUtils)
   ```

2. **更新包含路径**:
   ```cpp
   // 旧方式
   #include "xatomic.h"
   
   // 新方式
   #include "XAtomic/xatomic.h"
   ```

3. **更新命名空间**:
   ```cpp
   // 旧方式
   using namespace XUtils;
   
   // 新方式
   using namespace XUtils::XAtomic;
   ```

## 测试

### 运行测试

```bash
# 运行所有测试
./build_modern.sh --test

# 构建并运行测试
./build_modern.sh --debug --shared --test
```

### 测试覆盖

- 单元测试：每个模块都有对应的测试
- 集成测试：模块间交互测试
- 性能测试：关键功能的性能基准

## 贡献指南

### 开发环境设置

1. 克隆仓库
2. 安装依赖（Boost, Qt6）
3. 运行构建脚本
4. 编写测试

### 代码规范

- 使用 C++20 特性
- 遵循现代 C++ 最佳实践
- 添加适当的文档和注释
- 确保测试覆盖

## 版本历史

### v1.0.0 (当前版本)
- 完成模块化重构
- 实现现代化构建系统
- 添加简化链接方式
- 完善文档和测试

## 未来计划

- [ ] 添加更多平台支持
- [ ] 性能优化
- [ ] 更多工具模块
- [ ] 包管理器支持 