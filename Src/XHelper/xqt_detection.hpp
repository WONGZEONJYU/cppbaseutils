#ifndef X_QT_DETECTION_HPP
#define X_QT_DETECTION_HPP

// Qt自动检测机制
// 优先级：用户定义 > Qt宏 > 环境变量

// 1. 用户显式定义
#ifdef XUTILS_FORCE_QT
    #define HAS_QT
#elif defined(XUTILS_DISABLE_QT)
    #undef HAS_QT
#else
    // 2. 自动检测Qt宏
    #if defined(QT_VERSION) || defined(QT_CORE_LIB) || defined(QT_WIDGETS_LIB) || defined(QT_GUI_LIB)
        #define HAS_QT
    #endif
    
    // 3. 检测Qt版本宏
    #if defined(QT_VERSION_MAJOR) && (QT_VERSION_MAJOR >= 5)
        #define HAS_QT
    #endif
    
    // 4. 检测Qt模块宏
    #if defined(QT_CORE_LIB) || defined(QT_WIDGETS_LIB) || defined(QT_GUI_LIB) || \
        defined(QT_NETWORK_LIB) || defined(QT_SQL_LIB) || defined(QT_XML_LIB)
        #define HAS_QT
    #endif
#endif

// 定义Qt版本信息
#ifdef HAS_QT
    #ifdef QT_VERSION_MAJOR
        #define XUTILS_QT_VERSION_MAJOR QT_VERSION_MAJOR
    #else
        #define XUTILS_QT_VERSION_MAJOR 5  // 默认假设Qt5
    #endif
    
    #if XUTILS_QT_VERSION_MAJOR >= 6
        #define XUTILS_QT6
    #else
        #define XUTILS_QT5
    #endif
#endif

#endif // X_QT_DETECTION_HPP
