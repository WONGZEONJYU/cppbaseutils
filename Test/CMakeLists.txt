cmake_minimum_required(VERSION 3.31.6)

project(test)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../ cppbaseutils.out)

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/xlib")

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

file(GLOB H_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h*)
aux_source_directory(. SRC_FILES)
add_executable(${PROJECT_NAME} ${SRC_FILES} ${H_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE XLib)
