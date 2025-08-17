# XUtils 使用示例

## 简化链接方式示例

### 1. 基本使用

在您的项目中，现在可以使用简化的方式链接 XUtils 库：

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(MyProject)

# 查找 XUtils 库
find_package(XUtils REQUIRED)

# 创建您的可执行文件
add_executable(my_app main.cpp)

# 简化的链接方式 - 无需指定静态或动态版本
target_link_libraries(my_app
    XUtils
)
```

### 2. 库类型自动选择

系统会根据 XUtils 的构建配置自动选择库类型：

#### 场景 1：只构建了共享库
```bash
# 构建 XUtils 时只构建共享库
./build_modern.sh --release --shared
```

您的项目会自动链接到 `XUtils_shared`。

#### 场景 2：只构建了静态库
```bash
# 构建 XUtils 时只构建静态库
./build_modern.sh --release --static
```

您的项目会自动链接到 `XUtils_static`。

#### 场景 3：同时构建了两种库
```bash
# 构建 XUtils 时同时构建两种库
./build_modern.sh --release --both
```

您的项目会优先链接到 `XUtils_shared`。

### 3. 完整示例项目

#### 项目结构
```
my_project/
├── CMakeLists.txt
├── main.cpp
└── include/
    └── my_header.h
```

#### CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.20)
project(MyProject VERSION 1.0.0)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖库
find_package(XUtils REQUIRED)

# 创建可执行文件
add_executable(my_app
    main.cpp
    include/my_header.h
)

# 设置包含目录
target_include_directories(my_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 简化的链接方式
target_link_libraries(my_app
    XUtils
)
```

#### main.cpp
```cpp
#include <iostream>
#include "XAtomic/xatomic.h"
#include "XThreadPool/xthreadpool.h"
#include "XTools/xtools.h"

int main() {
    std::cout << "XUtils 简化链接示例" << std::endl;
    
    // 使用原子操作
    XUtils::XAtomic::Atomic<int> counter(0);
    counter.fetch_add(1);
    
    // 使用线程池
    XUtils::XThreadPool::ThreadPool pool(4);
    
    // 使用工具类
    auto ptr = XUtils::XTools::make_unique<int>(42);
    
    std::cout << "所有功能正常工作！" << std::endl;
    return 0;
}
```

### 4. 传统链接方式（仍然支持）

如果您需要明确指定库类型，仍然可以使用传统方式：

```cmake
# 明确指定共享库
target_link_libraries(my_app
    XUtils::XUtils_shared
)

# 明确指定静态库
target_link_libraries(my_app
    XUtils::XUtils_static
)
```

### 5. 构建和运行

#### 构建 XUtils
```bash
# 构建共享库版本
cd /path/to/XUtils
./build_modern.sh --release --shared --install

# 或构建静态库版本
./build_modern.sh --release --static --install
```

#### 构建您的项目
```bash
# 构建您的项目
cd /path/to/my_project
mkdir build && cd build
cmake ..
make
```

### 6. 优势

#### 简化使用
- 无需记住具体的库名称
- 自动选择最优的库类型
- 减少配置错误

#### 灵活性
- 支持不同的构建配置
- 向后兼容传统方式
- 易于迁移和维护

#### 现代化
- 遵循现代 CMake 最佳实践
- 使用接口目标
- 更好的依赖管理

### 7. 故障排除

#### 问题：找不到库
```bash
# 确保 XUtils 已正确安装
find_package(XUtils REQUIRED)
```

#### 问题：链接错误
```bash
# 检查库是否正确构建
ls /usr/local/lib/libXUtils*
```

#### 问题：运行时错误
```bash
# 确保动态库在路径中
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

### 8. 最佳实践

1. **使用简化链接方式**：除非有特殊需求，否则使用 `XUtils` 而不是具体的库名称
2. **设置正确的 C++ 标准**：确保使用 C++20 或更高版本
3. **正确处理依赖**：使用 `find_package` 而不是手动指定路径
4. **测试兼容性**：在不同构建配置下测试您的项目

这样的设计让使用 XUtils 变得更加简单和直观！ 