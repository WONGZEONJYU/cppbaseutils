
set(SRC_CMAKE_PATH ${PROJECT_SOURCE_DIR}/Src/cmake)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${SRC_CMAKE_PATH})

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_FILES)
file(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h*)

addQtTestExecutable(XQthreadTest
        "${SRC_FILES}"
        "${HEADER_FILES}"
        Qt_MODULES Core Gui Widgets
)
include(CopyQtDll)
copy_qt_windows_dlls(XQthreadTest Qt_MODULES Core Test Gui Widgets Network WebSockets Qml Quick)
