
function(addQtTestExecutable TargetName SRC_FILES HEADER_FILES)
    # 1. 定义关键字
    set(multi_value_args Qt_MODULES)

    # 2. 解析参数 (ARGN 指的是 TargetName 和 Qt_Dir 之后的所有参数)
    cmake_parse_arguments(ARG "" "" "${multi_value_args}" ${ARGN})

    # 3. 使用解析出来的变量：ARG_MODULES
    message(STATUS "    Creating target: ${TargetName}")
    message(STATUS "    Qt Directory: ${Qt_Dir}")
    message(STATUS "    ARG_Qt_MODULES = ${ARG_Qt_MODULES}")

    set(QtModelList)
    foreach(module ${ARG_Qt_MODULES})
        message(STATUS "    Linking Qt module: ${module}")
        list(APPEND QtModelList ${module})
    endforeach()

    find_package(Qt6 COMPONENTS "${QtModelList}" REQUIRED)

    set(QtModelLinkList "${QtModelList}")
    list(TRANSFORM QtModelLinkList PREPEND "Qt::")
    message(STATUS "   QtModelLinkList = ${QtModelLinkList}")

    add_executable(${TargetName} "${SRC_FILES}" "${HEADER_FILES}")

    target_link_libraries(${TargetName} ${PROJECT_NAME} "${QtModelLinkList}")

    qt_helper_load(${TargetName})

    set_target_properties(${TargetName} PROPERTIES  OUTPUT_NAME "${TargetName}")
    set_target_properties(${TargetName} PROPERTIES
            AUTOMOC ON
            AUTOUIC ON
            AUTORCC ON
            QT_NO_PRIVATE_MODULE_WARNING ON
    )

    if(BUILD_TESTING)
        enable_testing()
        add_test(NAME ${TargetName} COMMAND ${TargetName})
    endif()

endfunction()

# --- 调用方式 ---
#addQtTestExecutable(MyAppTest "xx.cpp" "xxx.hpp" "/path/to/qt"
#        Qt_MODULES Core Widgets Network Gui
#)

#[[
function(addQtTestTarget TargetName Qt_Dir)
    # ARGN 会包含除了 TargetName 和 Qt_Dir 以外的所有剩余参数
    set(QtModules ${ARGN})

    message(STATUS "Target: ${TargetName}")

    # 直接遍历
    foreach(module ${QtModules})
        message(STATUS "Adding module: ${module}")
    endforeach()
endfunction()

# --- 调用方式 ---
# 后面所有的参数都会被归为 QtModel 的范畴
addQtTestTarget(MyAppTest "/path/to/qt" Core Widgets Network)
]]
