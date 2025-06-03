cmake_minimum_required(VERSION 3.31.6)

project(test)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../ cppbaseutils.out)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

file(GLOB SRC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(${PROJECT_NAME} ${SRC_FILE} )

target_link_libraries(${PROJECT_NAME} PUBLIC XSignal XHelper)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

