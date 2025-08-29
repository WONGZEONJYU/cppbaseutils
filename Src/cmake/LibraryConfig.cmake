# 库配置函数

# 定义库的公共属性设置函数
function(configure_library_target target_name)
    # 配置符号可见性
    if(USE_SYMBOL_VISIBILITY)
        target_compile_definitions(${target_name} PUBLIC -DUSE_SYMBOL_VISIBILITY)
    endif()

    # 设置平台特定的宏定义
    if(WIN32)
        target_compile_definitions(${target_name} PUBLIC -DPLATFORM_WINDOWS)
        if(EXPORT_ALL_SYMBOLS)
            target_compile_definitions(${target_name} PUBLIC -DEXPORT_ALL_SYMBOLS)
        endif()
        if(AUTO_EXPORT_IMPORT)
            target_compile_definitions(${target_name} PUBLIC -DAUTO_EXPORT_IMPORT)
        endif()
    elseif(APPLE)
        target_compile_definitions(${target_name} PUBLIC -DPLATFORM_MACOS)
        if(AUTO_EXPORT_IMPORT)
            target_compile_definitions(${target_name} PUBLIC -DAUTO_EXPORT_IMPORT)
        endif()
    elseif(UNIX)
        target_compile_definitions(${target_name} PUBLIC -DPLATFORM_LINUX)
        if(AUTO_EXPORT_IMPORT)
            target_compile_definitions(${target_name} PUBLIC -DAUTO_EXPORT_IMPORT)
        endif()
    endif()

    # 为动态库构建时添加构建宏
    get_target_property(target_type ${target_name} TYPE)
    if(target_type STREQUAL "SHARED_LIBRARY")
        target_compile_definitions(${target_name} PRIVATE -DX_BUILDING_LIBRARY)
    endif()

    # 配置 Boost
    if(XUtils_Boost_FOUND)
        target_compile_definitions(${target_name} PUBLIC -DHAS_BOOST)
        target_include_directories(${target_name} PUBLIC ${Boost_INCLUDE_DIRS})
        target_link_libraries(${target_name} PUBLIC ${Boost_LIBRARIES})
    endif()

    # 配置 Qt - 自动检测Qt
    if(XUtils_Qt6_FOUND)
        # 传递Qt的宏定义，让库自动检测Qt
        target_compile_definitions(${target_name} PUBLIC 
            -DQT_VERSION_MAJOR=6
            -DQT_CORE_LIB
        )
        # 不链接Qt库，让用户自己链接
    elseif(XUtils_Qt5_FOUND)
        # 传递Qt的宏定义，让库自动检测Qt
        target_compile_definitions(${target_name} PUBLIC 
            -DQT_VERSION_MAJOR=5
            -DQT_CORE_LIB
        )
        # 不链接Qt库，让用户自己链接
    endif()

    # 链接线程库
    target_link_libraries(${target_name} PUBLIC Threads::Threads)

    # 指定库的包含目录
    target_include_directories(${target_name}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
endfunction() 