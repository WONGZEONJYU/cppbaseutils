# 测试配置

# 创建测试可执行文件
add_executable(${PROJECT_NAME}_test)

# 添加测试源文件
target_sources(${PROJECT_NAME}_test PRIVATE
    main.cpp
    main.hpp
)

# 链接库 - 现在可以使用简化的方式
target_link_libraries(${PROJECT_NAME}_test
    ${PROJECT_NAME}
)

# 设置测试可执行文件的输出名称
set_target_properties(${PROJECT_NAME}_test PROPERTIES
    OUTPUT_NAME "${PROJECT_NAME}_test"
)

# 添加测试
if(BUILD_TESTING)
    enable_testing()
    add_test(NAME ${PROJECT_NAME}_test COMMAND ${PROJECT_NAME}_test)
endif()
