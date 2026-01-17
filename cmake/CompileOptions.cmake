# 全局编译选项配置

# 设置编译器特定选项
if(MSVC)
    # MSVC 编译器选项
    add_compile_options(
        /utf-8                    # 使用UTF-8编码
        /Zc:__cplusplus          # 正确报告__cplusplus宏
        /std:c++latest           # 使用最新的C++标准
        /W4                      # 警告级别4
        /wd4251                  # 禁用dll-interface警告
        /wd4275                  # 禁用dll-interface警告
        /Zc:preprocessor         # 启用__VA_OPT__
        $<$<CONFIG:Release>:/Zi> # Release配置启用PDB编译
    )
    # 仅在 Release 配置下添加 /DEBUG（生成 PDB 链接信息）
    add_link_options($<$<CONFIG:Release>:/DEBUG>)
    # 根据构建类型设置优化选项
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(/Od /Zi /RTC1)
        add_link_options(/DEBUG)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(/O2 /Ob2 /DNDEBUG)
    elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        add_compile_options(/O2 /Ob2 /Zi /DNDEBUG)
        add_link_options(/DEBUG)
    elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        add_compile_options(/O1 /Ob1 /DNDEBUG)
    endif()
    
else()
    # GCC/Clang 编译器选项
    add_compile_options(
        -Wall                     # 启用所有警告
        -Wextra                   # 启用额外警告
        -Wpedantic               # 严格遵循标准
        -fPIC                    # 位置无关代码
    )

    # 根据构建类型设置优化选项
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0 -fno-omit-frame-pointer)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -DNDEBUG)
    elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        add_compile_options(-O2 -g -DNDEBUG)
    elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        add_compile_options(-Os -DNDEBUG)
    endif()
    
    # 符号可见性控制
    add_compile_options(-fvisibility=hidden)
    add_compile_options(-fvisibility-inlines-hidden)
endif()

# 设置平台特定的宏定义
if(WIN32)
    add_compile_definitions(PLATFORM_WINDOWS)
elseif(APPLE)
    add_compile_definitions(PLATFORM_MACOS)
elseif(UNIX)
    add_compile_definitions(PLATFORM_LINUX)
endif()

# 根据构建类型设置宏定义
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG_BUILD _DEBUG)
else()
    add_compile_definitions(RELEASE_BUILD NDEBUG)
endif()
