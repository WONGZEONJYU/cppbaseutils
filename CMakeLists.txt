cmake_minimum_required(VERSION 3.31.6)

project(cppbaseutils)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/XHelper)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/XThreadPool)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/XSignal)


