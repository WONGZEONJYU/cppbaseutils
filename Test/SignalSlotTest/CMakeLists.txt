# XObject 信号槽测试
cmake_minimum_required(VERSION 3.16)

# 创建测试可执行文件
add_executable(XObjectSignalSlotTest XObjectSignalSlotTest.cpp)

# 链接必要的库
target_link_libraries(XObjectSignalSlotTest PRIVATE ${PROJECT_NAME})

target_include_directories(XObjectSignalSlotTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# 如果需要，可以添加到测试套件
if(BUILD_TESTING)
    add_test(NAME XObjectSignalSlotTest COMMAND XObjectSignalSlotTest)
endif()
