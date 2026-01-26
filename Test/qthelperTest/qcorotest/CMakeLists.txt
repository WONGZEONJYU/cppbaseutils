set(SRC_CMAKE_PATH ${PROJECT_SOURCE_DIR}/Src/cmake)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${SRC_CMAKE_PATH})

set(QT_NO_PRIVATE_MODULE_WARNING ON)
set(CMAKE_AUTOMOC ON)

#-----------------------------------------------------------#
# Options
#-----------------------------------------------------------#

# Allow QCORO_BUILD_TESTING to override BUILD_TESTING, but default to BUILD_TESTING if not set
if (DEFINED QCORO_BUILD_TESTING)
    set(BUILD_TESTING ${QCORO_BUILD_TESTING})
endif()

option(QCORO_BUILD_EXAMPLES "Build examples" ON)
add_feature_info(Examples QCORO_BUILD_EXAMPLES "Build examples")
option(QCORO_BUILD_TESTING "Build QCoro tests" ${BUILD_TESTING})
#message(WARNING "BUILD_TESTING = ${BUILD_TESTING}")
add_feature_info(Testing QCORO_BUILD_TESTING "Build QCoro tests")
option(QCORO_ENABLE_ASAN "Build with AddressSanitizer" OFF)
add_feature_info(Asan QCORO_ENABLE_ASAN "Build with AddressSanitizer")
option(QCORO_DISABLE_DEPRECATED_TASK_H "Disable deprecated task.h header" OFF)

if(WIN32 OR APPLE OR ANDROID)
    option(QCORO_WITH_QTDBUS "Build QtDBus support" OFF)
else()
    option(QCORO_WITH_QTDBUS "Build QtDBus support" ON)
endif()

add_feature_info(QtDBus QCORO_WITH_QTDBUS "Build QtDBus support")
option(QCORO_WITH_QTNETWORK "Build QtNetwork support" ON)
add_feature_info(QtNetwork QCORO_WITH_QTNETWORK "Build QtNetwork support")
option(QCORO_WITH_QTWEBSOCKETS "Build QtWebSockets support" ON)
add_feature_info(QtWebSockets QCORO_WITH_QTWEBSOCKETS "Build QtWebSockets support")
option(QCORO_WITH_QTQUICK "Build QtQuick support" ON)
add_feature_info(QtQuick QCORO_WITH_QTQUICK "Build QtQuick support")
option(QCORO_WITH_QML "Build QML integration features" ON)
add_feature_info(QtQml QCORO_WITH_QML "Build QML integration features")
option(QCORO_WITH_QTTEST "Build QtTest support" ON)
add_feature_info(QtTest QCORO_WITH_QTTEST "Build QtTest support")

set(REQUIRED_QT_COMPONENTS Core)
set(REQUIRED_QT5_COMPONENTS)
set(REQUIRED_QT6_COMPONENTS)

if (QCORO_WITH_QTDBUS)
    list(APPEND REQUIRED_QT_COMPONENTS DBus)
endif()

if (QCORO_WITH_QTNETWORK)
    list(APPEND REQUIRED_QT_COMPONENTS Network)
endif()

if (QCORO_WITH_QTWEBSOCKETS)
    list(APPEND REQUIRED_QT_COMPONENTS WebSockets)
endif()

if (QCORO_WITH_QTQUICK)
    list(APPEND REQUIRED_QT_COMPONENTS Gui Quick QuickPrivate)
endif()

if (QCORO_WITH_QML)
    list(APPEND REQUIRED_QT_COMPONENTS Qml)
    # Qt6 needs access to private API
    list(APPEND REQUIRED_QT6_COMPONENTS QmlPrivate)
endif()

if (QCORO_WITH_QTTEST)
    list(APPEND REQUIRED_QT_COMPONENTS Test)
endif()

if (QCORO_BUILD_EXAMPLES)
    list(APPEND REQUIRED_QT_COMPONENTS Widgets Concurrent)
endif()

if (BUILD_TESTING)
    list(APPEND REQUIRED_QT_COMPONENTS Test Concurrent)
endif()

set(MIN_REQUIRED_QT5_VERSION "5.12")
set(MIN_REQUIRED_QT6_VERSION "6.2.0")

include(QCoroFindQt)

qcoro_find_qt(
        QT_VERSION "${USE_QT_VERSION}"
        COMPONENTS "${REQUIRED_QT_COMPONENTS}"
        QT5_COMPONENTS "${REQUIRED_QT5_COMPONENTS}"
        QT6_COMPONENTS "${REQUIRED_QT6_COMPONENTS}"
        FOUND_VER_VAR QT_VERSION_MAJOR
)

include(CodeCoverage)
add_code_coverage()
add_code_coverage_all_targets(EXCLUDE "${CMAKE_BINARY_DIR}" "Test/qthelperTest/qcorotest/testexec/*")

include(AddQCoroLibrary)
include(CopyQtDll)

if (QCORO_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if (QCORO_BUILD_TESTING)
    add_subdirectory(testlibs)
    add_subdirectory(testexec)
endif ()
