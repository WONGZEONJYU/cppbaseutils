# 收集源文件
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_FILES)
file(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h*)

# 过滤掉 CMakeLists.txt 文件
list(FILTER SRC_FILES EXCLUDE REGEX "CMakeLists\\.txt$")
list(FILTER HEADER_FILES EXCLUDE REGEX "CMakeLists\\.txt$")

set(Observer Observer)

# 创建主测试可执行文件
add_executable(${Observer})

# 添加主测试源文件
target_sources(${Observer} PRIVATE ${SRC_FILES} ${HEADER_FILES})

# 链接库 - 现在可以使用简化的方式
target_link_libraries(${Observer} PRIVATE ${PROJECT_NAME})

target_include_directories(${Observer} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# 设置测试可执行文件的输出名称
set_target_properties(${Observer} PROPERTIES
        OUTPUT_NAME "${Observer}"
)

# 添加测试
if(BUILD_TESTING)
    enable_testing()
    add_test(NAME ${Observer} COMMAND ${Observer})
endif()

unset(Observer)
