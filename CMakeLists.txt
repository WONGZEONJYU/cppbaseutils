cmake_minimum_required(VERSION 3.20)

project(XCppBaseUtilsLib)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Src)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Test)

#set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/XCppBaseUtils")
#-DCMAKE_INSTALL_PREFIX="${CMAKE_CURRENT_BINARY_DIR}/XCppBaseUtils"