cmake_minimum_required(VERSION 3.20)

project(XCppBaseUtilsTest)

#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../Src cppbaseutils.out)

#set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/XCppBaseUtils")

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

file(GLOB H_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h*)
aux_source_directory(. SRC_FILES)
add_executable(${PROJECT_NAME} ${SRC_FILES} ${H_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE XCppBaseUtils_shared)
