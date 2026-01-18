include_guard()
function(xqt_helper_load TARGET_NAME XQtHelper_INCLUDE_DIRS)

    if (NOT TARGET Qt${QT_VERSION_MAJOR}::Core)
        message(WARNING "QT not found!")
        return()
    endif ()

    #message(STATUS "    XQtHelper_INCLUDE_DIRS = ${XQtHelper_INCLUDE_DIRS}")

    file(GLOB_RECURSE Concurrency CONFIGURE_DEPENDS "${XQtHelper_INCLUDE_DIRS}/concurrency/*.h*")
    file(GLOB_RECURSE QCoroCore CONFIGURE_DEPENDS "${XQtHelper_INCLUDE_DIRS}/qcoro/core/*.h*")
    list(APPEND HELPER_HEADER "${Concurrency}" "${QCoroCore}")

    if (TARGET Qt${QT_VERSION_MAJOR}::Network)
        file(GLOB_RECURSE Network "${XQtHelper_INCLUDE_DIRS}/qcoro/network/*.h*")
        list(APPEND HELPER_HEADER "${Network}")
    endif()

    if (TARGET Qt${QT_VERSION_MAJOR}::Qml)

        if (NOT TARGET Qt${QT_VERSION_MAJOR}::QmlPrivate)
            message(WARNING "Please link to the QmlPrivate module")
        endif ()

        file(GLOB_RECURSE QML "${XQtHelper_INCLUDE_DIRS}/qcoro/qml/*.h*")
        list(APPEND HELPER_HEADER "${QML}")
    endif ()

    if (TARGET Qt${QT_VERSION_MAJOR}::Quick)
        file(GLOB_RECURSE Quick "${XQtHelper_INCLUDE_DIRS}/qcoro/quick/*.h*")
        list(APPEND HELPER_HEADER "${Quick}")
    endif ()

    if (TARGET Qt${QT_VERSION_MAJOR}::WebSockets)
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
