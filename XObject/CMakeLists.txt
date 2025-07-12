cmake_minimum_required(VERSION 3.20)

project(XObject)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

# 为所有可能的库目标添加源文件
if (TARGET XCppBaseUtils_shared)
    target_sources(XCppBaseUtils_shared PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/xobject.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/xobject_p.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/xobject.hpp
    )
endif ()

if (TARGET XCppBaseUtils_static)
    target_sources(XCppBaseUtils_static PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/xobject.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/xobject_p.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/xobject.hpp
    )
endif ()
