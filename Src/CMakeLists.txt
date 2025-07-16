cmake_minimum_required(VERSION 3.20)

# 定义项目名称、版本和语言
project(XCppBaseUtils VERSION 1.0 LANGUAGES CXX)

#set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/XCppBaseUtils")

# 设置 C++ 标准
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_DEBUG_LIBS ON)
set(Boost_DEBUG ON)

# 添加选项来控制构建哪种类型的库
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_STATIC_LIBS "Build static libraries" OFF)  # 默认关闭静态库

# 确保至少构建一种类型的库
if(NOT BUILD_SHARED_LIBS AND NOT BUILD_STATIC_LIBS)
    message(FATAL_ERROR "At least one of BUILD_SHARED_LIBS or BUILD_STATIC_LIBS must be ON")
endif()

# 查找 Boost
find_package(Boost QUIET COMPONENTS ALL)

# 创建库目标列表
set(LIBRARY_TARGETS)

# 创建共享库
if(BUILD_SHARED_LIBS)
    add_library(${PROJECT_NAME}_shared SHARED)
    set_target_properties(${PROJECT_NAME}_shared PROPERTIES
            OUTPUT_NAME ${PROJECT_NAME}
            CLEAN_DIRECT_OUTPUT 1
    )
    list(APPEND LIBRARY_TARGETS ${PROJECT_NAME}_shared)
endif()

# 创建静态库
if(BUILD_STATIC_LIBS)
    add_library(${PROJECT_NAME}_static STATIC)
    set_target_properties(${PROJECT_NAME}_static PROPERTIES
            OUTPUT_NAME ${PROJECT_NAME}
            CLEAN_DIRECT_OUTPUT 1
    )
    list(APPEND LIBRARY_TARGETS ${PROJECT_NAME}_static)
endif()

# 包含子目录
add_subdirectory(XAtomic)
add_subdirectory(XGlobal)
add_subdirectory(XHelper)
add_subdirectory(XObject)
add_subdirectory(XSignal)
add_subdirectory(XThreadPool)
add_subdirectory(XTools)

# 定义库的公共属性设置函数
function(configure_library_target target_name)
    # 设置编译器特定选项
    if (MSVC AND WIN32)
        target_compile_options(${target_name} PUBLIC /std:c++latest)
        target_compile_options(${target_name} PUBLIC /Zc:__cplusplus)
        target_compile_options(${target_name} PUBLIC "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    endif()

    # 配置 Boost
    if(Boost_FOUND)
        target_compile_definitions(${target_name} PUBLIC -DHAS_BOOST)
        target_include_directories(${target_name} PUBLIC ${Boost_INCLUDE_DIRS})
        target_link_libraries(${target_name} PUBLIC ${Boost_LIBRARIES})
    endif()

    # 指定库的包含目录
    target_include_directories(${target_name}
            PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<INSTALL_INTERFACE:include>
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/XObject
    )
endfunction()

# 配置所有库目标
foreach(target IN LISTS LIBRARY_TARGETS)
    configure_library_target(${target})
endforeach()

# 安装库
install(TARGETS ${LIBRARY_TARGETS}
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
        PATTERN "*_p.hpp" EXCLUDE
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
        NAMESPACE XCppBaseUtils::
        DESTINATION lib/cmake/${PROJECT_NAME}
)

# 生成 CMake 配置文件
include(CMakePackageConfigHelpers)

# 创建包配置文件
configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        INSTALL_DESTINATION lib/cmake/${PROJECT_NAME}
        PATH_VARS CMAKE_INSTALL_PREFIX
)

write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)

# 安装配置文件
install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION lib/cmake/${PROJECT_NAME}
)
