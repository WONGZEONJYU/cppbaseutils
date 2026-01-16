include_guard()

function(add_xutils_library src_files header_files include_directories)

    # 过滤掉不需要编译到库中的文件
    list(FILTER src_files EXCLUDE REGEX "CMakeLists\\.txt$")
    list(FILTER src_files EXCLUDE REGEX "example\\.cpp$")
    list(FILTER header_files EXCLUDE REGEX "CMakeLists\\.txt$")

    #[[
    message(STATUS "include_directories = ${include_directories}")
    foreach (item IN LISTS header_files) message(STATUS "item = ${item}")  endforeach ()
    ]]

    foreach(target IN LISTS LIBRARY_TARGETS)
        if(TARGET ${target})
            target_sources(${target} PRIVATE ${src_files} ${header_files})
            target_include_directories(${target} PRIVATE ${include_directories} )
        endif()
    endforeach()

endfunction()
