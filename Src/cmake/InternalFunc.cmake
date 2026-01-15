set(XQtHelper_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/XQtHelper CACHE PATH "QThelper header")

function(xqt_helper_load TARGET_NAME)

    list(APPEND HELPER_HEADER "${XQtHelper_INCLUDE_DIRS}/concurrency/xqthread.hpp")
    list(APPEND HELPER_HEADER "${XQtHelper_INCLUDE_DIRS}/coro/private/waitsignalhelper.hpp")

    foreach (item ${HELPER_HEADER})
        if(NOT EXISTS ${item})
            message(WARNING "LibXUnits2: Could not find ${item}" file)
            return()
        endif ()
    endforeach ()

    target_sources(${TARGET_NAME} PRIVATE "${HELPER_HEADER}")
    set_target_properties(${TARGET_NAME} PROPERTIES AUTOMOC ON)

endfunction()
