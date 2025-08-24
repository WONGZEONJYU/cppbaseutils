# XUtils 构建脚本使用说明

## 概述

`build_modern.sh` 是一个现代化的 CMake 构建脚本，支持多种构建配置和依赖管理选项。

## 主要修复

### 1. 多配置构建修复
- **问题**: 原脚本无法正确同时构建 Debug 和 Release 版本
- **修复**: 改进了多配置构建逻辑，确保 Debug 和 Release 版本都能正确构建
- **验证**: 使用 `--multi` 选项可以同时构建两个版本

### 2. 依赖管理选项
- **新增**: `--disable-qt` 和 `--enable-qt` 选项
- **新增**: `--disable-boost` 和 `--enable-boost` 选项
- **功能**: 可以灵活控制是否使用 Qt 和 Boost 依赖

### 3. 构建逻辑改进
- **错误处理**: 改进了错误检查和退出逻辑
- **状态反馈**: 提供更详细的构建状态信息
- **文件显示**: 改进了生成文件的显示逻辑

## 使用方法

### 基本构建选项

```bash
# 构建 Debug 版本
./build_modern.sh --debug

# 构建 Release 版本
./build_modern.sh --release

# 同时构建 Debug 和 Release 版本
./build_modern.sh --multi

# 清理并构建
./build_modern.sh --clean --debug
```

### 库类型选项

```bash
# 构建共享库
./build_modern.sh --debug --shared

# 构建静态库
./build_modern.sh --release --static

# 同时构建两种类型
./build_modern.sh --multi --both
```

### 依赖控制选项

```bash
# 禁用 Qt 依赖
./build_modern.sh --debug --disable-qt

# 禁用 Boost 依赖
./build_modern.sh --release --disable-boost

# 同时禁用两个依赖
./build_modern.sh --debug --disable-qt --disable-boost

# 强制启用 Qt 依赖
./build_modern.sh --debug --enable-qt
```

### 其他选项

```bash
# 详细输出
./build_modern.sh --debug --verbose

# 构建并安装
./build_modern.sh --release --install

# 构建并运行测试
./build_modern.sh --debug --test

# 组合使用
./build_modern.sh --multi --both --disable-qt --test --install
```

## 构建目录结构

### 单配置构建
```
build/
├── bin/
├── lib/
└── CMakeCache.txt
```

### 多配置构建
```
build/multi/
├── bin/
│   ├── Debug/
│   └── Release/
├── lib/
│   ├── Debug/
│   └── Release/
└── CMakeCache.txt
```

## 依赖状态

脚本会在构建过程中显示依赖状态摘要：

```
=== 依赖状态摘要 ===
Boost: TRUE
Qt: FALSE
Threads: TRUE
=====================
```

## 错误处理

- 脚本使用 `set -e` 确保遇到错误时立即退出
- 每个构建步骤都有错误检查
- 提供详细的错误信息和状态反馈

## 测试脚本

使用 `test_build.sh` 可以测试脚本的各种功能：

```bash
./test_build.sh
```

测试包括：
- 帮助信息显示
- Debug/Release 构建
- 多配置构建
- 依赖禁用选项
- 详细输出模式

## 注意事项

1. **权限**: 确保脚本有执行权限 (`chmod +x build_modern.sh`)
2. **依赖**: 如果不禁用 Qt 或 Boost，系统需要安装相应的库
3. **清理**: 使用 `--clean` 选项可以清理之前的构建文件
4. **多配置**: `--multi` 选项会创建 `build/multi` 目录

## 故障排除

### 常见问题

1. **权限错误**: 运行 `chmod +x build_modern.sh`
2. **依赖缺失**: 使用 `--disable-qt` 或 `--disable-boost` 选项
3. **构建失败**: 检查 CMake 错误信息，可能需要安装缺失的依赖

### 调试模式

使用 `--verbose` 选项可以获得详细的构建信息：

```bash
./build_modern.sh --debug --verbose
```

这将显示：
- 完整的 CMake 命令
- 详细的构建过程
- 依赖查找信息 