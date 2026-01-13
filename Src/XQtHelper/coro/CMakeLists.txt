# Coro 模块

# 收集源文件
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_FILES)

file(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h*)
file(GLOB IMPL_HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/impl/*.h*)
list(APPEND HEADER_FILES ${IMPL_HEADER_FILES})

# 过滤掉不需要编译到库中的文件
list(FILTER SRC_FILES EXCLUDE REGEX "CMakeLists\\.txt$")
list(FILTER SRC_FILES EXCLUDE REGEX "example\\.cpp$")
list(FILTER HEADER_FILES EXCLUDE REGEX "CMakeLists\\.txt$")

# 为所有库目标添加源文件
foreach(target IN LISTS LIBRARY_TARGETS)
    if(TARGET ${target})
        target_sources(${target} PRIVATE
                ${SRC_FILES}
                ${HEADER_FILES}
        )

        # 设置包含目录
        target_include_directories(${target} PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif()
endforeach()
