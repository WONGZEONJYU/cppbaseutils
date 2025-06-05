cmake_minimum_required(VERSION 3.31.6)

project(XObject)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)


target_sources(XLib PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/xobject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/xobject_p.hpp
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/xobject.hpp
)
