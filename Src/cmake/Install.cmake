# 安装配置

# 安装库目标（包括统一的接口目标）
install(TARGETS ${LIBRARY_TARGETS} ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# 安装头文件，保留子目录结构，排除私有头文件
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/XAtomic/
    DESTINATION include/XAtomic
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*_p.hpp" EXCLUDE
    PATTERN "*_private.hpp" EXCLUDE
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/XGlobal/
    DESTINATION include/XGlobal
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*_p.hpp" EXCLUDE
    PATTERN "*_private.hpp" EXCLUDE
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/XHelper/
    DESTINATION include/XHelper
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*_p.hpp" EXCLUDE
    PATTERN "*_private.hpp" EXCLUDE
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/XObject/
    DESTINATION include/XObject
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*_p.hpp" EXCLUDE
    PATTERN "*_private.hpp" EXCLUDE
)

# 平台特定的模块
if(APPLE OR UNIX)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/XSignal/
        DESTINATION include/XSignal
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "*_p.hpp" EXCLUDE
        PATTERN "*_private.hpp" EXCLUDE
    )
endif()

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/XThreadPool/
    DESTINATION include/XThreadPool
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*_p.hpp" EXCLUDE
    PATTERN "*_private.hpp" EXCLUDE
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/XTools/
    DESTINATION include/XTools
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*_p.hpp" EXCLUDE
    PATTERN "*_private.hpp" EXCLUDE
)

# 导出库配置供其他项目使用
install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION lib/cmake/${PROJECT_NAME}
)

# 生成 CMake 配置文件
include(CMakePackageConfigHelpers)

# 设置配置文件需要的变量
if(Boost_FOUND)
    set(XUtils_Boost_FOUND "TRUE")
else()
    set(XUtils_Boost_FOUND "FALSE")
endif()

if(Qt6_FOUND)
    set(XUtils_Qt6_FOUND "TRUE")
else()
    set(XUtils_Qt6_FOUND "FALSE")
endif()

if(Qt5_FOUND)
    set(XUtils_Qt5_FOUND "TRUE")
else()
    set(XUtils_Qt5_FOUND "FALSE")
endif()

# 创建包配置文件
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION lib/cmake/${PROJECT_NAME}
    PATH_VARS CMAKE_INSTALL_PREFIX
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# 安装配置文件
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION lib/cmake/${PROJECT_NAME}
) 