# XUtils 库构建配置

include(cmake/AddXUntilsLibrary.cmake)

# 创建库目标列表
set(LIBRARY_TARGETS)

# 创建共享库
if(BUILD_SHARED_LIBS)
    add_library(${PROJECT_NAME}_shared SHARED)
    
    # 根据构建类型设置库名称
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set_target_properties(${PROJECT_NAME}_shared PROPERTIES
            OUTPUT_NAME "${PROJECT_NAME}d"
            CLEAN_DIRECT_OUTPUT 1
        )
    else()
        set_target_properties(${PROJECT_NAME}_shared PROPERTIES
            OUTPUT_NAME "${PROJECT_NAME}"
            CLEAN_DIRECT_OUTPUT 1
        )
    endif()
    
    target_compile_definitions(${PROJECT_NAME}_shared PUBLIC 
        -DX_BUILDING_LIBRARY
        -DX_SHARED
    )

    # 配置符号可见性
    if(USE_SYMBOL_VISIBILITY AND (CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
        target_compile_options(${PROJECT_NAME}_shared PRIVATE -fvisibility=hidden)
    endif()

    list(APPEND LIBRARY_TARGETS ${PROJECT_NAME}_shared)
endif()

# 创建静态库
if(BUILD_STATIC_LIBS)
    add_library(${PROJECT_NAME}_static STATIC)
    
    # 根据构建类型设置库名称
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set_target_properties(${PROJECT_NAME}_static PROPERTIES
            OUTPUT_NAME "${PROJECT_NAME}d"
            CLEAN_DIRECT_OUTPUT 1
        )
    else()
        set_target_properties(${PROJECT_NAME}_static PROPERTIES
            OUTPUT_NAME "${PROJECT_NAME}"
            CLEAN_DIRECT_OUTPUT 1
        )
    endif()
    
    target_compile_definitions(${PROJECT_NAME}_static PUBLIC -DX_STATIC)
    list(APPEND LIBRARY_TARGETS ${PROJECT_NAME}_static)
endif()

# 包含子目录
set(folderNamesList
    XAtomic XGlobal XHelper XLog
    XThreadPool XMath XMemory XContainerHelper
    XContainer XTupleHelper XDesignPattern XQtHelper
    XConcurrentQueue
)

foreach (name IN LISTS folderNamesList)
    add_subdirectory(${name})
endforeach ()

add_subdirectory(XCoroutine)

# 平台特定的模块
if(APPLE OR UNIX)
    add_subdirectory(Unix)
elseif (WIN32)
    add_subdirectory(Win)
endif()

# 包含库配置
include(cmake/LibraryConfig.cmake)

# 配置所有库目标
foreach(target IN LISTS LIBRARY_TARGETS)
    configure_library_target(${target})
endforeach()

# 创建统一的接口目标
add_library(${PROJECT_NAME} INTERFACE)

# 根据构建选项选择链接的库
if(BUILD_SHARED_LIBS AND BUILD_STATIC_LIBS)
    # 如果同时构建静态和动态库，根据BUILD_SHARED_LIBS的值决定
    if(BUILD_SHARED_LIBS)
        target_link_libraries(${PROJECT_NAME} INTERFACE ${PROJECT_NAME}_shared)
        message(STATUS "Using shared library for ${PROJECT_NAME} interface target")
    else()
        target_link_libraries(${PROJECT_NAME} INTERFACE ${PROJECT_NAME}_static)
        message(STATUS "Using static library for ${PROJECT_NAME} interface target")
    endif()
elseif(BUILD_SHARED_LIBS)
    target_link_libraries(${PROJECT_NAME} INTERFACE ${PROJECT_NAME}_shared)
    message(STATUS "Using shared library for ${PROJECT_NAME} interface target")
elseif(BUILD_STATIC_LIBS)
    target_link_libraries(${PROJECT_NAME} INTERFACE ${PROJECT_NAME}_static)
    message(STATUS "Using static library for ${PROJECT_NAME} interface target")
endif()

# 安装配置
if(ENABLE_INSTALL)
    include(cmake/Install.cmake)
endif()

include(cmake/InternalFunc.cmake)
