cmake_minimum_required(VERSION 3.31.6)

# 定义项目名称、版本和语言
project(XLib VERSION 1.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
#add_definitions(-DUNUSE_STD_THREAD_LOCAL)
# 定义动态库
add_library(${PROJECT_NAME} SHARED)

# 包含子目录
add_subdirectory(XAtomic)
add_subdirectory(XGlobal)
add_subdirectory(XHelper)
add_subdirectory(XObject)
add_subdirectory(XSignal)
add_subdirectory(XThreadPool)
add_subdirectory(XTools)

# 指定库的包含目录
# PUBLIC 包含目录用于暴露给测试和外部调用者
# PRIVATE 包含目录仅用于构建 XLib，确保 xobject_p.hpp 可用
target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/XObject # 确保 xobject_p.hpp 在构建 XLib 时可用
)

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/xlib")

# 安装规则
install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}Targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)

# 安装头文件，保留子目录结构，排除带 _p 的文件
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/XAtomic/
        DESTINATION include/XAtomic
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "*_p.hpp" EXCLUDE
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/XGlobal/
        DESTINATION include/XGlobal
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "*_p.hpp" EXCLUDE
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/XHelper/
        DESTINATION include/XHelper
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "*_p.hpp" EXCLUDE
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/XObject/
        DESTINATION include/XObject
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "*_p.hpp" EXCLUDE # 排除 xobject_p.hpp
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/XSignal/
        DESTINATION include/XSignal
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "*_p.hpp" EXCLUDE
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/XThreadPool/
        DESTINATION include/XThreadPool
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "*_p.hpp" EXCLUDE
)

# 导出库配置供其他项目使用
install(EXPORT ${PROJECT_NAME}Targets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE XLib::
        DESTINATION lib/cmake/${PROJECT_NAME}
)

# 生成 CMake 配置文件
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION lib/cmake/${PROJECT_NAME}
)
