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

# 添加选项来控制符号导出
option(EXPORT_ALL_SYMBOLS "Export all symbols from shared libraries" ON)   # 默认开启全符号导出
option(USE_SYMBOL_VISIBILITY "Use symbol visibility attributes" ON)  # 默认启用符号可见性控制
option(AUTO_EXPORT_IMPORT "Automatically handle export/import based on build context" ON)  # 自动处理导出导入

# 确保至少构建一种类型的库
if(NOT BUILD_SHARED_LIBS AND NOT BUILD_STATIC_LIBS)
    message(FATAL_ERROR "At least one of BUILD_SHARED_LIBS or BUILD_STATIC_LIBS must be ON")
endif()

# 查找 Boost
find_package(Boost COMPONENTS ALL QUIET)
find_package(Qt6 COMPONENTS Core QUIET)
find_package(Qt5 COMPONENTS Core QUIET)

# 创建库目标列表
set(LIBRARY_TARGETS)

# 创建共享库
if(BUILD_SHARED_LIBS)
    add_library(${PROJECT_NAME}_shared SHARED)
    set_target_properties(${PROJECT_NAME}_shared PROPERTIES
            OUTPUT_NAME ${PROJECT_NAME}
            CLEAN_DIRECT_OUTPUT 1
    )
    target_compile_definitions(${PROJECT_NAME}_shared PUBLIC -DX_SHARED)

    # 配置符号导出
    if(WIN32)
        if(EXPORT_ALL_SYMBOLS)
            set_target_properties(${PROJECT_NAME}_shared PROPERTIES
                    WINDOWS_EXPORT_ALL_SYMBOLS ON
            )
        endif()
    endif()

    list(APPEND LIBRARY_TARGETS ${PROJECT_NAME}_shared)
endif()

# 创建静态库
if(BUILD_STATIC_LIBS)
    add_library(${PROJECT_NAME}_static STATIC)
    set_target_properties(${PROJECT_NAME}_static PROPERTIES
            OUTPUT_NAME ${PROJECT_NAME}
            CLEAN_DIRECT_OUTPUT 1
    )
    target_compile_definitions(${PROJECT_NAME}_static PUBLIC -DX_STATIC)
    list(APPEND LIBRARY_TARGETS ${PROJECT_NAME}_static)
endif()

# 包含子目录
add_subdirectory(XAtomic)
add_subdirectory(XGlobal)
add_subdirectory(XHelper)
add_subdirectory(XObject)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "Mac OS X" OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_subdirectory(XSignal)
endif()

add_subdirectory(XThreadPool)
add_subdirectory(XTools)

# 定义库的公共属性设置函数
function(configure_library_target target_name)
    # 设置编译器特定选项
    if(MSVC AND WIN32)
        #target_compile_options(${target_name} PUBLIC /std:c++20)
        target_compile_options(${target_name} PUBLIC /std:c++latest)
        target_compile_options(${target_name} PUBLIC /Zc:__cplusplus)
        target_compile_options(${target_name} PUBLIC "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    endif()

    # 配置符号可见性
    if(USE_SYMBOL_VISIBILITY)
        target_compile_definitions(${target_name} PUBLIC -DUSE_SYMBOL_VISIBILITY)

        # 设置默认符号可见性为隐藏 (GCC/Clang)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${target_name} PRIVATE -fvisibility=hidden)
            target_compile_options(${target_name} PRIVATE -fvisibility-inlines-hidden)
        endif()
    endif()

    # 设置平台特定的宏定义
    if(WIN32)
        target_compile_definitions(${target_name} PUBLIC -DPLATFORM_WINDOWS)
        if(EXPORT_ALL_SYMBOLS)
            target_compile_definitions(${target_name} PUBLIC -DEXPORT_ALL_SYMBOLS)
        endif()
        if(AUTO_EXPORT_IMPORT)
            target_compile_definitions(${target_name} PUBLIC -DAUTO_EXPORT_IMPORT)
        endif()
    elseif(APPLE)
        target_compile_definitions(${target_name} PUBLIC -DPLATFORM_MACOS)
        if(AUTO_EXPORT_IMPORT)
            target_compile_definitions(${target_name} PUBLIC -DAUTO_EXPORT_IMPORT)
        endif()
    elseif(UNIX)
        target_compile_definitions(${target_name} PUBLIC -DPLATFORM_LINUX)
        if(AUTO_EXPORT_IMPORT)
            target_compile_definitions(${target_name} PUBLIC -DAUTO_EXPORT_IMPORT)
        endif()
    endif()

    # 为动态库构建时添加构建宏
    get_target_property(target_type ${target_name} TYPE)
    if(target_type STREQUAL "SHARED_LIBRARY")
        target_compile_definitions(${target_name} PRIVATE -DX_BUILDING_LIBRARY)
    endif()

    # 配置 Boost
    if(Boost_FOUND)
        target_compile_definitions(${target_name} PUBLIC -DHAS_BOOST)
        target_include_directories(${target_name} PUBLIC ${Boost_INCLUDE_DIRS})
        target_link_libraries(${target_name} PUBLIC ${Boost_LIBRARIES})
    endif()

    # 配置 Qt
    if(Qt6_FOUND OR Qt5_FOUND)
        set(CMAKE_AUTOMOC ON)
        set(CMAKE_AUTORCC ON)
        set(CMAKE_AUTOUIC ON)
        target_compile_definitions(${target_name} PUBLIC -DHAS_QT)
        target_link_libraries(${target_name} PUBLIC Qt::Core)
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

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "Mac OS X" OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/XSignal/
            DESTINATION include/XSignal
            FILES_MATCHING
            PATTERN "*.h"
            PATTERN "*.hpp"
            PATTERN "*_p.hpp" EXCLUDE
    )
endif()

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
