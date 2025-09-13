cmake_minimum_required(VERSION 3.20)

# 设置项目名称、版本和描述
project(XUtils 
    VERSION 2.0.0
    DESCRIPTION "C++ Base Utilities Library"
    LANGUAGES CXX
    HOMEPAGE_URL "https://github.com/yourusername/XUtils"
)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置默认构建类型
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS 
        "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# 输出构建信息
message(STATUS "Project: ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")

# 设置全局编译选项
include(cmake/CompileOptions.cmake)

# 设置全局选项
include(cmake/Options.cmake)

# 查找依赖包
include(cmake/Dependencies.cmake)

# 添加测试子目录
add_subdirectory(Src)
add_subdirectory(Test)
