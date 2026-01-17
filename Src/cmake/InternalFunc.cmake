include_guard()
include(cmake/ExternalFunc.cmake)

macro(qt_helper_load)
endmacro()

set(XQtHelper_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/XQtHelper CACHE PATH "QT_helper_header")

macro(qt_helper_load TARGET_NAME)
    xqt_helper_load(${TARGET_NAME} ${XQtHelper_INCLUDE_DIRS})
endmacro()

unset(XQtHelper_INCLUDE_DIRS)
