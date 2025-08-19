# 测试配置

# 创建主测试可执行文件
add_executable(${PROJECT_NAME}_test)

# 添加主测试源文件
target_sources(${PROJECT_NAME}_test PRIVATE
    main.cpp
    main.hpp
)

# 创建XLog测试可执行文件
add_executable(${PROJECT_NAME}_xlog_test)

# 添加XLog测试源文件
target_sources(${PROJECT_NAME}_xlog_test PRIVATE
    test_xlog.cpp
)

# 链接库 - 现在可以使用简化的方式
target_link_libraries(${PROJECT_NAME}_test
    ${PROJECT_NAME}
)

target_link_libraries(${PROJECT_NAME}_xlog_test
    ${PROJECT_NAME}
)

# 设置测试可执行文件的输出名称
set_target_properties(${PROJECT_NAME}_test PROPERTIES
    OUTPUT_NAME "${PROJECT_NAME}_test"
)

set_target_properties(${PROJECT_NAME}_xlog_test PROPERTIES
    OUTPUT_NAME "${PROJECT_NAME}_xlog_test"
)

# 添加测试
if(BUILD_TESTING)
    enable_testing()
    add_test(NAME ${PROJECT_NAME}_test COMMAND ${PROJECT_NAME}_test)
    add_test(NAME ${PROJECT_NAME}_xlog_test COMMAND ${PROJECT_NAME}_xlog_test)
endif()
