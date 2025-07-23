cmake_minimum_required(VERSION 3.20)

project(XCppBaseUtilsTest)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB H_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h*)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_FILES)
add_executable(${PROJECT_NAME} ${SRC_FILES} ${H_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE XCppBaseUtils_shared)
