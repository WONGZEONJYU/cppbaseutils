# XMemory 模块

# 收集源文件
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_FILES)
file(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h*)
add_xutils_library("${SRC_FILES}" "${HEADER_FILES}" "${CMAKE_CURRENT_SOURCE_DIR}")
