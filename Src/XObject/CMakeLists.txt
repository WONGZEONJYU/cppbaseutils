cmake_minimum_required(VERSION 3.20)

project(XObject)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

# XObject 模块 - 使用手动文件列表以确保精确控制

# 定义源文件
set(XOBJECT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/xobject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/xfunctionaltools_impl.cpp
)

# 定义头文件
set(XOBJECT_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/xfunctionaltools_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/xsignalslot.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/xobject.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/xobjectdefs_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/xobject_p.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/xobject_p_p.hpp
)

# 为所有库目标添加源文件
foreach(target IN LISTS LIBRARY_TARGETS)
    if(TARGET ${target})
        target_sources(${target} PRIVATE
            ${XOBJECT_SOURCES}
            ${XOBJECT_HEADERS}
        )
        
        # 设置包含目录
        target_include_directories(${target} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif()
endforeach()
