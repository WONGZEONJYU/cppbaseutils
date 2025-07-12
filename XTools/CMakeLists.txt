cmake_minimum_required(VERSION 3.20)

project(XTools)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

aux_source_directory(. SRC_FILS)
file(GLOB HEAD_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h*)

if (TARGET XCppBaseUtils_shared)
    target_sources(XCppBaseUtils_shared PRIVATE
        ${SRC_FILS}
        ${HEAD_FILES}
    )
endif ()

if (TARGET XCppBaseUtils_static)
    target_sources(XCppBaseUtils_static PRIVATE
        ${SRC_FILS}
        ${HEAD_FILES}
    )
endif ()
