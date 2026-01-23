add_executable(timer-example main.cpp)
target_link_libraries(timer-example
    #QCoro${QT_VERSION_MAJOR}Core
    ${PROJECT_NAME}
    Qt${QT_VERSION_MAJOR}::Core
)
qt_helper_load(timer-example)
set_target_defaults(timer-example)
