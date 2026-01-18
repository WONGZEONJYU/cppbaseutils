
set(Qt6_DIR)
if(APPLE OR UNIX)
    list(APPEND Qt6_DIR "$ENV{HOME}/Qt/6.10.1/macos")
elseif (WIN32)
    list(APPEND Qt6_DIR "C:/Qt/6.10.1/msvc2022_64")
endif()

add_subdirectory(xqthread)
add_subdirectory(testlibs)

unset(Qt6_DIR)
