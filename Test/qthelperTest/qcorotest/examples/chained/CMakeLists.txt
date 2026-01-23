add_executable(chained-example main.cpp)
target_link_libraries(chained-example
    #QCoro${QT_VERSION_MAJOR}Core
        ${PROJECT_NAME}
    Qt${QT_VERSION_MAJOR}::Core
)
qt_helper_load(chained-example)
set_target_defaults(chained-example)
