cmake_minimum_required(VERSION 3.20)

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

add_xutils_library("${XOBJECT_SOURCES}" "${XOBJECT_HEADERS}" "${CMAKE_CURRENT_SOURCE_DIR}")
