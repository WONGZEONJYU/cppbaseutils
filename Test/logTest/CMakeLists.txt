# 收集源文件
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_FILES)
file(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h*)

# 过滤掉 CMakeLists.txt 文件
list(FILTER SRC_FILES EXCLUDE REGEX "CMakeLists\\.txt$")
list(FILTER HEADER_FILES EXCLUDE REGEX "CMakeLists\\.txt$")

set(LogTest LogTest)

# 创建XLog测试可执行文件
add_executable(${LogTest})

# 添加XLog测试源文件
target_sources(${LogTest} PRIVATE ${SRC_FILES} ${HEADER_FILES})

target_include_directories(${LogTest} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${LogTest} ${PROJECT_NAME})

set_target_properties(${LogTest} PROPERTIES
        OUTPUT_NAME "${LogTest}"
)

# 添加测试
if(BUILD_TESTING)
    enable_testing()
    add_test(NAME ${LogTest} COMMAND ${LogTest})
endif()
