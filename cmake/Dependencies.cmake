# 依赖包查找配置

# 设置 CMake 策略以消除 Boost 查找警告
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# 添加选项来禁用 Boost
option(DISABLE_BOOST "Disable Boost dependency" OFF)

# 设置Boost查找选项
if(NOT DISABLE_BOOST)
    set(Boost_USE_MULTITHREADED ON)
    set(Boost_USE_DEBUG_LIBS ON)
    set(Boost_DEBUG OFF)

    # 查找Boost
    find_package(Boost QUIET)
    if(Boost_FOUND)
        message(STATUS "Boost found: ${Boost_VERSION}")
        message(STATUS "Boost libraries: ${Boost_LIBRARIES}")
        message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
    else()
        message(WARNING "Boost not found - some features may be disabled")
    endif()
else()
    message(STATUS "Boost disabled by user option")
    set(Boost_FOUND FALSE)
endif()

# 添加选项来禁用 Qt
option(DISABLE_QT "Disable Qt dependency" OFF)

# Qt路径配置
set(QT_PATHS)
if(NOT DISABLE_QT)
    if(APPLE)
        # macOS Qt路径
        list(APPEND QT_PATHS 
            "$ENV{HOME}/Qt/6.9.1/macos"
            "$ENV{HOME}/Qt/6.8.0/macos"
            "$ENV{HOME}/Qt/6.7.0/macos"
            "/usr/local/Qt-6.9.1"
            "/usr/local/Qt-6.8.0"
            "/usr/local/Qt-6.7.0"
        )
    elseif(WIN32)
        # Windows Qt路径
        list(APPEND QT_PATHS
            "C:/Qt6/6.9.1/msvc2022_64"
            "C:/Qt6/6.8.0/msvc2022_64"
            "C:/Qt6/6.7.0/msvc2022_64"
            "C:/Qt/6.9.1/msvc2022_64"
            "C:/Qt/6.8.0/msvc2022_64"
            "C:/Qt/6.7.0/msvc2022_64"
        )
    elseif(UNIX)
        # Linux Qt路径
        list(APPEND QT_PATHS
            "/usr/local/Qt-6.9.1"
            "/usr/local/Qt-6.8.0"
            "/usr/local/Qt-6.7.0"
            "/opt/Qt/6.9.1"
            "/opt/Qt/6.8.0"
            "/opt/Qt/6.7.0"
        )
    endif()

    # 查找Qt6
    find_package(Qt6 COMPONENTS Core QUIET PATHS ${QT_PATHS})
    if(Qt6_FOUND)
        message(STATUS "Qt6 found: ${Qt6_VERSION}")
        set(CMAKE_AUTOMOC ON)
        set(CMAKE_AUTORCC ON)
        set(CMAKE_AUTOUIC ON)
    else()
        # 如果Qt6没找到，尝试Qt5
        find_package(Qt5 COMPONENTS Core QUIET PATHS ${QT_PATHS})
        if(Qt5_FOUND)
            message(STATUS "Qt5 found: ${Qt5_VERSION}")
            set(CMAKE_AUTOMOC ON)
            set(CMAKE_AUTORCC ON)
            set(CMAKE_AUTOUIC ON)
        else()
            message(WARNING "Qt not found - Qt-dependent features will be disabled")
        endif()
    endif()
else()
    message(STATUS "Qt disabled by user option")
    set(Qt6_FOUND FALSE)
    set(Qt5_FOUND FALSE)
endif()

# 查找其他可能的依赖
find_package(Threads REQUIRED)
if(Threads_FOUND)
    message(STATUS "Threads library found")
endif()

# 设置全局变量供其他模块使用
set(XUtils_Boost_FOUND ${Boost_FOUND})
set(XUtils_Qt6_FOUND ${Qt6_FOUND})
set(XUtils_Qt5_FOUND ${Qt5_FOUND})
set(XUtils_Qt_FOUND ${Qt6_FOUND} OR ${Qt5_FOUND})

# 输出依赖状态摘要
message(STATUS "=== 依赖状态摘要 ===")
message(STATUS "Boost: ${XUtils_Boost_FOUND}")
message(STATUS "Qt: ${XUtils_Qt_FOUND}")
message(STATUS "Threads: ${Threads_FOUND}")
message(STATUS "=====================") 