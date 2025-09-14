# XObject 信号槽测试
cmake_minimum_required(VERSION 3.16)

# 创建测试可执行文件
add_executable(XObjectSignalSlotTest ../XObjectSignalSlotTest.cpp)

# 链接必要的库
target_link_libraries(XObjectSignalSlotTest 
    PRIVATE 
    XUtils_shared
)

# 设置编译选项
target_compile_features(XObjectSignalSlotTest PRIVATE cxx_std_17)

# 如果需要，可以添加到测试套件
if(BUILD_TESTING)
    add_test(NAME XObjectSignalSlotTest COMMAND XObjectSignalSlotTest)
endif()
