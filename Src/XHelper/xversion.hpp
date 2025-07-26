#ifndef X_NAMESPACE_HPP
#define X_NAMESPACE_HPP 1

//XTD_VERSION "0.0.1"

#define XTD_VERSION_MAJOR 0

#define XTD_VERSION_MINOR 0

#define XTD_VERSION_PATCH 1

#define XTD_VERSION_CHECK(major, minor, patch) ((major<<16) | (minor<<8) | (patch))

#define XTD_VERSION XTD_VERSION_CHECK(XTD_VERSION_MAJOR,XTD_VERSION_MINOR,XTD_VERSION_PATCH)

#define XTD_NAMESPACE_BEGIN namespace xtd {
#define XTD_NAMESPACE_END }

#define XTD_INLINE_NAMESPACE_BEGIN(name) inline namespace name {
#define XTD_INLINE_NAMESPACE_END XTD_NAMESPACE_END

//平台检测宏
#if defined(_WIN32) || defined(_WIN64)
    #define X_PLATFORM_WINDOWS
#elif defined(__APPLE__)
    #define X_PLATFORM_MACOS
#elif defined(__linux__)
    #define X_PLATFORM_LINUX
#else
    #define X_PLATFORM_UNKNOWN
#endif

// 编译器检测
#if defined(_MSC_VER)
    #define X_COMPILER_MSVC
#elif defined(__GNUC__)
    #define X_COMPILER_GCC
#elif defined(__clang__)
    #define X_COMPILER_CLANG
#endif

// 自动符号导出/导入系统
#ifdef AUTO_EXPORT_IMPORT

    #ifdef X_PLATFORM_WINDOWS
        // Windows 平台自动导出/导入
        #ifdef EXPORT_ALL_SYMBOLS
            // 使用 WINDOWS_EXPORT_ALL_SYMBOLS,所有符号自动导出
            #define X_EXPORT
            #define X_IMPORT
            #define X_API
        #else
            // 传统方式:构建时导出，使用时导入
            #ifdef X_BUILDING_LIBRARY
                #define X_EXPORT __declspec(dllexport)
                #define X_API __declspec(dllexport)
            #else
                #define X_EXPORT __declspec(dllimport)
                #define X_API __declspec(dllimport)
            #endif
            #define X_IMPORT __declspec(dllimport)
        #endif
        #define X_LOCAL

    #elif defined(X_PLATFORM_MACOS) || defined(X_PLATFORM_LINUX)
        // macOS 和 Linux 平台自动导出
        #ifdef USE_SYMBOL_VISIBILITY
            #if defined(X_COMPILER_GCC) || defined(X_COMPILER_CLANG)
                #define X_EXPORT __attribute__((visibility("default")))
                #define X_IMPORT __attribute__((visibility("default")))
                #define X_API __attribute__((visibility("default")))
                #define X_LOCAL  __attribute__((visibility("hidden")))
            #else
                #define X_EXPORT
                #define X_IMPORT
                #define X_API
                #define X_LOCAL
            #endif
        #else
            // 不使用符号可见性控制，所有符号默认可见
            #define X_EXPORT
            #define X_IMPORT
            #define X_API
            #define X_LOCAL
        #endif

    #else
        // 未知平台
        #define X_EXPORT
        #define X_IMPORT
        #define X_API
        #define X_LOCAL
    #endif

#else  // 手动控制模式(保持向后兼容)

    #ifdef X_PLATFORM_WINDOWS
    //Windows 平台
        #ifdef EXPORT_ALL_SYMBOLS
            #define X_EXPORT
            #define X_IMPORT
            #define X_LOCAL
        #else
        #ifdef X_SHARED
            #ifdef X_BUILDING_LIBRARY
                #define X_EXPORT __declspec(dllexport)
            #else
                #define X_EXPORT __declspec(dllimport)
            #endif
        #else
            #define X_EXPORT
        #endif
        #define X_IMPORT __declspec(dllimport)
        #define X_LOCAL
    #endif

    #elif defined(X_PLATFORM_MACOS) || defined(X_PLATFORM_LINUX)
    // macOS 和 Linux 平台
        #ifdef USE_SYMBOL_VISIBILITY
            #if defined(X_COMPILER_GCC) || defined(X_COMPILER_CLANG)
                #define X_EXPORT __attribute__((visibility("default")))
                #define X_IMPORT __attribute__((visibility("default")))
                #define X_LOCAL  __attribute__((visibility("hidden")))
            #else
                #define X_EXPORT
                #define X_IMPORT
                #define X_LOCAL
            #endif
        #else
            #define X_EXPORT
            #define X_IMPORT
            #define X_LOCAL
        #endif
    #else
        // 未知平台
        #define X_EXPORT
        #define X_IMPORT
        #define X_LOCAL
#endif

// 手动模式下的 API 宏
#ifdef X_STATIC
    #define X_API
#else
    #define X_API X_EXPORT
#endif

#endif // AUTO_EXPORT_IMPORT

// 通用宏定义（不受模式影响）

// 类导出宏
#define X_CLASS_EXPORT X_API

// 模板导出宏（主要用于 Windows）
#ifdef X_PLATFORM_WINDOWS
    #define X_TEMPLATE_EXPORT X_API
#else
    #define X_TEMPLATE_EXPORT
#endif

// 静态库处理
#ifdef X_STATIC
// 静态库重新定义所有宏为空
    #undef X_API
    #undef X_CLASS_EXPORT
    #undef X_TEMPLATE_EXPORT
    #define X_API
    #define X_CLASS_EXPORT
    #define X_TEMPLATE_EXPORT
#endif

// 废弃符号宏
#ifdef X_COMPILER_MSVC
    #define X_DEPRECATED __declspec(deprecated)
    #define X_DEPRECATED_MSG(msg) __declspec(deprecated(msg))
#elif defined(X_COMPILER_GCC) || defined(X_COMPILER_CLANG)
    #define X_DEPRECATED __attribute__((deprecated))
    #define X_DEPRECATED_MSG(msg) __attribute__((deprecated(msg)))
#else
    #define X_DEPRECATED
    #define X_DEPRECATED_MSG(msg)
#endif

// 内联宏
#ifdef X_COMPILER_MSVC
    #define X_FORCE_INLINE __forceinline
    #define X_NO_INLINE __declspec(noinline)
#elif defined(X_COMPILER_GCC) || defined(X_COMPILER_CLANG)
    #define X_FORCE_INLINE __attribute__((always_inline)) inline
    #define X_NO_INLINE __attribute__((noinline))
#else
    #define X_FORCE_INLINE inline
    #define X_NO_INLINE
#endif

// 调用约定宏 (主要用于 Windows)
#ifdef X_PLATFORM_WINDOWS
    #define X_STDCALL __stdcall
    #define X_CDECL   __cdecl
    #define X_FASTCALL __fastcall
#else
    #define X_STDCALL
    #define X_CDECL
    #define X_FASTCALL
#endif

// 便利宏：用于变量的导出/导入声明
#ifdef AUTO_EXPORT_IMPORT
    #ifdef X_PLATFORM_WINDOWS
        #ifndef EXPORT_ALL_SYMBOLS
            #ifdef X_BUILDING_LIBRARY
                #define X_DECLARE_VARIABLE(type, name) X_API extern type name
                #define X_DEFINE_VARIABLE(type, name, value) type name = value
            #else
                #define X_DECLARE_VARIABLE(type, name) X_API extern type name
                #define X_DEFINE_VARIABLE(type, name, value) extern type name
            #endif
        #else
            #define X_DECLARE_VARIABLE(type, name) extern type name
            #define X_DEFINE_VARIABLE(type, name, value) type name = value
        #endif
    #else
        #define X_DECLARE_VARIABLE(type, name) X_API extern type name
        #define X_DEFINE_VARIABLE(type, name, value) type name = value
    #endif
#else
// 手动模式下的变量宏
    #define X_DECLARE_VARIABLE(type, name) X_EXPORT extern type name
    #define X_DEFINE_VARIABLE(type, name, value) type name = value
#endif

#endif // X_EXPORT_H
