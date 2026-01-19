include_guard()
function(xqt_helper_load TARGET_NAME XQtHelper_INCLUDE_DIRS)

    function(target_links_qt_module target module result)
        get_target_property(_libs ${target} LINK_LIBRARIES)
        get_target_property(_iface_libs ${target} INTERFACE_LINK_LIBRARIES)

        set(_all_libs ${_libs} ${_iface_libs})

        list(FIND _all_libs Qt${QT_VERSION_MAJOR}::${module} _idx)

        if(_idx GREATER -1)
            set(${result} TRUE PARENT_SCOPE)
        else()
            set(${result} FALSE PARENT_SCOPE)
        endif()
    endfunction()

    target_links_qt_module(${TARGET_NAME} Core HASCore)
    if (NOT HASCore)
        message(WARNING "QT not found!")
        return()
    endif ()

    #message(STATUS "    XQtHelper_INCLUDE_DIRS = ${XQtHelper_INCLUDE_DIRS}")

    file(GLOB_RECURSE Concurrency CONFIGURE_DEPENDS "${XQtHelper_INCLUDE_DIRS}/concurrency/*.h*")
    file(GLOB_RECURSE QCoroCore CONFIGURE_DEPENDS "${XQtHelper_INCLUDE_DIRS}/qcoro/core/*.h*")
    list(APPEND HELPER_HEADER "${Concurrency}" "${QCoroCore}")

    target_links_qt_module(${TARGET_NAME} Network HASNetwork)
    if (HASNetwork)
        file(GLOB_RECURSE Network "${XQtHelper_INCLUDE_DIRS}/qcoro/network/*.h*")
        list(APPEND HELPER_HEADER "${Network}")
    endif()

    target_links_qt_module(${TARGET_NAME} Qml HASQml)
    target_links_qt_module(${TARGET_NAME} QmlPrivate HASQmlPrivate)
    if (HASQml AND HASQmlPrivate)
        file(GLOB_RECURSE QML "${XQtHelper_INCLUDE_DIRS}/qcoro/qml/*.h*")
        list(APPEND HELPER_HEADER "${QML}")
    endif ()

    target_links_qt_module(${TARGET_NAME} Quick HASQuick)
    target_links_qt_module(${TARGET_NAME} QuickPrivate HASQuickPrivate)
    if (HASQuick AND HASQuickPrivate)
        file(GLOB_RECURSE Quick "${XQtHelper_INCLUDE_DIRS}/qcoro/quick/*.h*")
        list(APPEND HELPER_HEADER "${Quick}")
    endif ()

    target_links_qt_module(${TARGET_NAME} WebSockets HASWebSockets)
    if (HASWebSockets)
        file(GLOB_RECURSE WebSockets "${XQtHelper_INCLUDE_DIRS}/qcoro/websockets/*.h*")
        list(APPEND HELPER_HEADER ${WebSockets})
    endif ()

    #message(STATUS "HELPER_HEADER = ${HELPER_HEADER}")

    foreach (item IN LISTS ${HELPER_HEADER})
        if(NOT EXISTS ${item})
            message(WARNING "LibXUnits2: Could not find ${item} file" )
            return()
        endif ()
    endforeach ()

    target_sources(${TARGET_NAME} PRIVATE "${HELPER_HEADER}")
    set_target_properties(${TARGET_NAME} PROPERTIES AUTOMOC ON)

endfunction()

macro(qt_helper_load TARGET_NAME)
    message(STATUS "    XUnits2_XQtHelper_INCLUDE_DIRS = ${XUnits2_XQtHelper_INCLUDE_DIRS}")
    xqt_helper_load(${TARGET_NAME} ${XUnits2_XQtHelper_INCLUDE_DIRS})
endmacro()
