cmake_minimum_required(VERSION 3.31.6)

project(XThreadPool)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

#[[
set(BUILD_SHARED_LIBS ON)
aux_source_directory(. SRC_FILES)
file(GLOB HEADS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h*)
add_library(${PROJECT_NAME} ${SRC_FILES} ${HEADS_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
]]

aux_source_directory(. SRC_FILS)
file(GLOB HEAD_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h*)

target_sources(XLib PRIVATE
    ${SRC_FILS}
    ${HEAD_FILES}
)
