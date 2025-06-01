cmake_minimum_required(VERSION 3.31.6)

project(cppbaseutils)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

add_subdirectory(XHelper)
add_subdirectory(XThreadPool)
add_subdirectory(XSignal)
add_subdirectory(XThread)
add_subdirectory(XObject)