# XCppBaseUtils 自动符号导出导入构建说明

## 新增的 CMake 选项

### 1. EXPORT_ALL_SYMBOLS
- **默认值**: ON (已更改为默认开启)
- **作用**: 控制是否导出动态库的所有符号
- **适用平台**: 主要用于 Windows
- **说明**: 启用后，Windows 平台会自动导出所有符号，无需手动添加 `__declspec(dllexport)`

### 2. USE_SYMBOL_VISIBILITY
- **默认值**: ON
- **作用**: 控制是否使用符号可见性属性
- **适用平台**: 主要用于 macOS 和 Linux (GCC/Clang)
- **说明**: 启用后会设置默认符号可见性为隐藏，只有标记为导出的符号才可见

### 3. AUTO_EXPORT_IMPORT（新增）
- **默认值**: ON
- **作用**: 自动处理符号的导出和导入
- **适用平台**: 所有平台
- **说明**: 启用后，编译库时自动导出符号，使用库时自动导入符号，无需手动控制

## 自动化特性

### 核心优势
1. **零配置使用**：使用库时无需定义任何宏或特殊设置
2. **自动上下文识别**：CMake 自动识别是在构建库还是使用库
3. **跨平台一致**：相同的代码在所有平台上都能正确工作
4. **向后兼容**：支持传统的手动控制模式

### 自动化机制
```cpp
// 构建库时：X_API 自动变为导出宏
// 使用库时：X_API 自动变为导入宏（Windows）或保持可见（Unix）

class X_CLASS_EXPORT MyClass {
public:
    X_API void myMethod();  // 自动处理导出/导入
};
```

## 构建命令示例

### 推荐用法（自动化模式）
```bash
# 所有平台通用 - 自动处理导出导入
cmake -B build -DBUILD_SHARED_LIBS=ON
cmake --build build --config Release

# 也可以显式启用自动化（默认已开启）
cmake -B build -DBUILD_SHARED_LIBS=ON -DAUTO_EXPORT_IMPORT=ON
cmake --build build --config Release
```

### Windows (MSVC)
```bash
# 方式1：使用 Windows 全符号导出（推荐）
cmake -B build -DBUILD_SHARED_LIBS=ON -DEXPORT_ALL_SYMBOLS=ON
cmake --build build --config Release

# 方式2：使用传统导出导入控制
cmake -B build -DBUILD_SHARED_LIBS=ON -DEXPORT_ALL_SYMBOLS=OFF
cmake --build build --config Release
```

### macOS/Linux (GCC/Clang)
```bash
# 默认配置即可（自动启用符号可见性控制）
cmake -B build -DBUILD_SHARED_LIBS=ON
cmake --build build --config Release

# 如需关闭符号可见性控制
cmake -B build -DBUILD_SHARED_LIBS=ON -DUSE_SYMBOL_VISIBILITY=OFF
cmake --build build --config Release
```

### 静态库构建
```bash
# 构建静态库
cmake -B build -DBUILD_SHARED_LIBS=OFF -DBUILD_STATIC_LIBS=ON
cmake --build build --config Release

# 同时构建动态库和静态库
cmake -B build -DBUILD_SHARED_LIBS=ON -DBUILD_STATIC_LIBS=ON
cmake --build build --config Release
```

## 使用说明

### 1. 最简单的使用方式（推荐）
```cpp
#include "XGlobal/x_export.h"

// 导出类 - 自动处理
class X_CLASS_EXPORT MyClass {
public:
    X_API void publicMethod();    // 自动导出/导入
private:
    X_LOCAL void privateMethod(); // 仅库内部可见
};

// 导出函数 - 自动处理
X_API void myFunction();

// 导出变量 - 使用便利宏
X_DECLARE_VARIABLE(int, myGlobalVar);  // 头文件中声明
// 在源文件中定义：
// X_DEFINE_VARIABLE(int, myGlobalVar, 42);
```

### 2. 构建库时无需特殊设置
```cpp
// 源文件：my_library.cpp
#include "my_header.h"

// 无需定义 X_BUILDING_LIBRARY，CMake 自动处理
void MyClass::publicMethod() {
    // 实现代码
}

X_DEFINE_VARIABLE(int, myGlobalVar, 42);  // 定义全局变量
```

### 3. 使用库时无需特殊设置
```cpp
// 客户端代码：main.cpp
#include "my_header.h"

int main() {
    MyClass obj;           // 自动导入类
    obj.publicMethod();    // 自动导入方法
    myFunction();          // 自动导入函数
    
    std::cout << myGlobalVar << std::endl;  // 自动导入变量
    return 0;
}
```

## 平台特性对比

| 特性 | Windows | macOS | Linux |
|------|---------|-------|-------|
| 默认符号可见性 | 隐藏 | 可见 | 可见 |
| 导出控制方式 | `__declspec(dllexport/dllimport)` | `__attribute__((visibility))` | `__attribute__((visibility))` |
| 全符号导出 | 支持 (WINDOWS_EXPORT_ALL_SYMBOLS) | 不适用 | 不适用 |
| 符号可见性控制 | 不适用 | 支持 (-fvisibility=hidden) | 支持 (-fvisibility=hidden) |

## 最佳实践建议

### Windows 平台
- 对于简单项目：启用 `EXPORT_ALL_SYMBOLS=ON`
- 对于复杂项目：使用手动导出控制，精确控制 API 接口

### macOS/Linux 平台
- 推荐启用 `USE_SYMBOL_VISIBILITY=ON`
- 使用 `X_LOCAL` 标记内部实现函数，减少符号污染

### 跨平台项目
- 使用提供的宏系统，确保代码在所有平台上正确工作
- 为公共 API 使用 `X_API` 宏
- 为内部实现使用 `X_LOCAL` 宏

## 调试符号导出

### Windows
```bash
# 查看 DLL 导出符号
dumpbin /EXPORTS your_library.dll

# 查看 LIB 文件符号
dumpbin /SYMBOLS your_library.lib
```

### macOS/Linux
```bash
# 查看动态库符号
nm -D your_library.so  # Linux
nm -D your_library.dylib  # macOS

# 查看所有符号（包括隐藏的）
nm your_library.so
objdump -T your_library.so  # Linux 详细信息
```

## 故障排除

### 常见问题
1. **链接错误 "unresolved external symbol"**
   - 检查是否正确设置了导出宏
   - 确认 `X_BUILDING_LIBRARY` 宏在构建库时定义

2. **符号未导出**
   - Windows: 检查是否使用了正确的 `__declspec` 或启用了 `EXPORT_ALL_SYMBOLS`
   - macOS/Linux: 检查是否正确使用了可见性属性

3. **符号冲突**
   - 使用 `X_LOCAL` 标记内部符号
   - 启用符号可见性控制减少符号污染