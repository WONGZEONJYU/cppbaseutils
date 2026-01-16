set(TargetName QCorotest)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

if(APPLE OR UNIX)
    list(APPEND Qt6_DIR "$ENV{HOME}/Qt/6.10.1/macos")
elseif (WIN32)
    list(APPEND Qt6_DIR "$ENV{USERPROFILE}/Qt/6.10.1/msvc2022_64")
endif()

set(QtMode)
list(APPEND QtModel Core Gui Widgets DBus)
find_package(Qt6 COMPONENTS ${QtModel} REQUIRED PATHS ${Qt6_DIR})

# 收集源文件
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_FILES)
file(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h*)

# 过滤掉 CMakeLists.txt 文件
list(FILTER SRC_FILES EXCLUDE REGEX "CMakeLists\\.txt$")
list(FILTER HEADER_FILES EXCLUDE REGEX "CMakeLists\\.txt$")

# 创建主测试可执行文件
add_executable(${TargetName})
qt_helper_load(${TargetName})

# 添加主测试源文件
target_sources(${TargetName} PRIVATE ${SRC_FILES} ${HEADER_FILES})

set(QtModelLink ${QtModel})
list(TRANSFORM QtModelLink PREPEND "Qt::")

# 链接库 - 现在可以使用简化的方式
target_link_libraries(${TargetName} ${PROJECT_NAME} ${QtModelLink})

target_include_directories(${TargetName} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# 设置测试可执行文件的输出名称
set_target_properties(${TargetName} PROPERTIES
        OUTPUT_NAME "${TargetName}"
)

# 添加测试
if(BUILD_TESTING)
    enable_testing()
    add_test(NAME ${TargetName} COMMAND ${TargetName})
endif()

unset(TargetName)
unset(Qt6Dir)
unset(QtMode)
