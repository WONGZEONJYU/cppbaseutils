function(copy_qt_windows_dlls TARGET_NAME)

    # 1. 定义关键字
    set(multi_value_args Qt_MODULES)
    # 2. 解析参数 (ARGN 指的是 TargetName 和 Qt_Dir 之后的所有参数)
    cmake_parse_arguments(ARG "" "" "${multi_value_args}" ${ARGN})

    if (WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        set(DEBUG_SUFFIX)
        if (MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
            set(DEBUG_SUFFIX "d")
        endif ()

        # 设置Qt DLL路径
        set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
        if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
            set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
            if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
                set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
            endif ()
        endif ()

        # 复制Qt平台插件
        if (EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E make_directory
                    "$<TARGET_FILE_DIR:${TARGET_NAME}>/plugins/platforms/")
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
                    "$<TARGET_FILE_DIR:${TARGET_NAME}>/plugins/platforms/")
        endif ()

        # 复制Qt库DLL
        foreach (QT_LIB ${ARG_Qt_MODULES})
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    "${QT_INSTALL_PATH}/bin/Qt6${QT_LIB}${DEBUG_SUFFIX}.dll"
                    "$<TARGET_FILE_DIR:${TARGET_NAME}>")
        endforeach (QT_LIB)
    endif ()
endfunction()
