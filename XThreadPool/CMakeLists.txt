cmake_minimum_required(VERSION 3.31.6)

project(XThreadPool)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

aux_source_directory(. SRC_FILES)
add_library(${PROJECT_NAME} ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
