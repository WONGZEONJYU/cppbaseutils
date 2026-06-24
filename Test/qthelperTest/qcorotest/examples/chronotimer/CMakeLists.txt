add_executable(chronoTimer-example main.cpp)
target_link_libraries(chronoTimer-example
        #QCoro${QT_VERSION_MAJOR}Core
        ${PROJECT_NAME}
        Qt${QT_VERSION_MAJOR}::Core
)
qt_helper_load(chronoTimer-example)
set_target_defaults(chronoTimer-example)
