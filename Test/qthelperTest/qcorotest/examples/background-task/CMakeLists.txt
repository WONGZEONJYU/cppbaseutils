add_executable(background-task-example)
target_sources(background-task-example PRIVATE main.cpp)
target_link_libraries(background-task-example
    PRIVATE
    #QCoro${QT_VERSION_MAJOR}Core
        ${PROJECT_NAME}
    Qt${QT_VERSION_MAJOR}::Core
)
qt_helper_load(background-task-example)
set_target_defaults(background-task-example)
