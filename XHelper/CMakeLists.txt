cmake_minimum_required(VERSION 3.31.6)

project(XHelper)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

#target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set(XHelper_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
