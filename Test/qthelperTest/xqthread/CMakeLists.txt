
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_FILES)
file(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h*)

addQtTestExecutable(XQthreadTest
        "${SRC_FILES}"
        "${HEADER_FILES}"
        Qt_MODULES Core Gui Widgets
)
