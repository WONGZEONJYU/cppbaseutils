include_guard()
function(xqt_helper_load TARGET_NAME XQtHelper_INCLUDE_DIRS)

    list(APPEND HELPER_HEADER "${XQtHelper_INCLUDE_DIRS}/concurrency/xqthread.hpp")
    list(APPEND HELPER_HEADER "${XQtHelper_INCLUDE_DIRS}/qcoro/core/private/waitsignalhelper.hpp")
    list(APPEND HELPER_HEADER "${XQtHelper_INCLUDE_DIRS}/qcoro/core/qcorothread.hpp")

    foreach (item ${HELPER_HEADER})
        if(NOT EXISTS ${item})
            message(WARNING "LibXUnits2: Could not find ${item} file" )
            return()
        endif ()
    endforeach ()

    target_sources(${TARGET_NAME} PRIVATE "${HELPER_HEADER}")
    set_target_properties(${TARGET_NAME} PROPERTIES AUTOMOC ON)

endfunction()

macro(qt_helper_load TARGET_NAME)
    message(STATUS "XUnits2_XQtHelper_INCLUDE_DIRS = ${XUnits2_XQtHelper_INCLUDE_DIRS}")
    xqt_helper_load(${TARGET_NAME} ${XUnits2_XQtHelper_INCLUDE_DIRS})
endmacro()
