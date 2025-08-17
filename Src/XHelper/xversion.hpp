#ifndef X_VERSION_HPP
#define X_VERSION_HPP

// ============================================================================
// 版本信息
// ============================================================================
#define X_VERSION_MAJOR 1
#define X_VERSION_MINOR 0
#define X_VERSION_PATCH 0

#define X_VERSION_STRING "1.0.0"
#define X_VERSION_CHECK(major, minor, patch) ((major << 16) | (minor << 8) | (patch))
#define X_VERSION X_VERSION_CHECK(X_VERSION_MAJOR, X_VERSION_MINOR, X_VERSION_PATCH)

// 向后兼容的版本宏
#define XTD_VERSION_MAJOR X_VERSION_MAJOR
#define XTD_VERSION_MINOR X_VERSION_MINOR
#define XTD_VERSION_PATCH X_VERSION_PATCH
#define XTD_VERSION X_VERSION
#define XTD_VERSION_CHECK X_VERSION_CHECK

// ============================================================================
// 命名空间宏
// ============================================================================
#define X_NAMESPACE_BEGIN namespace XUtils {
#define X_NAMESPACE_END }

// 向后兼容的宏定义
#define XTD_NAMESPACE_BEGIN X_NAMESPACE_BEGIN
#define XTD_NAMESPACE_END X_NAMESPACE_END
#define XTD_INLINE_NAMESPACE_BEGIN(name) inline namespace name {
#define XTD_INLINE_NAMESPACE_END }

// ============================================================================
// 平台检测
// ============================================================================
#if defined(_WIN32) || defined(_WIN64)
    #define X_PLATFORM_WINDOWS
#elif defined(__APPLE__)
    #define X_PLATFORM_MACOS
#elif defined(__linux__)
    #define X_PLATFORM_LINUX
#else
    #define X_PLATFORM_UNKNOWN
#endif

// ============================================================================
// 编译器检测
// ============================================================================
#if defined(_MSC_VER)
    #define X_COMPILER_MSVC
#elif defined(__GNUC__)
    #define X_COMPILER_GCC
#elif defined(__clang__)
    #define X_COMPILER_CLANG
#endif

// ============================================================================
// 符号导出/导入宏
// ============================================================================

// 静态库模式：所有符号都可见
#ifdef X_STATIC
    #define X_API
    #define X_EXPORT
    #define X_IMPORT
    #define X_LOCAL
    #define X_CLASS_EXPORT
    #define X_TEMPLATE_EXPORT

// 动态库模式
#else
    #ifdef X_PLATFORM_WINDOWS
        // Windows 平台
        #ifdef X_BUILDING_LIBRARY
            #define X_EXPORT __declspec(dllexport)
            #define X_API __declspec(dllexport)
        #else
            #define X_EXPORT __declspec(dllimport)
            #define X_API __declspec(dllimport)
        #endif
        #define X_IMPORT __declspec(dllimport)
        #define X_LOCAL

    #elif defined(X_PLATFORM_MACOS) || defined(X_PLATFORM_LINUX)
        // macOS 和 Linux 平台
        #ifdef USE_SYMBOL_VISIBILITY
            #if defined(X_COMPILER_GCC) || defined(X_COMPILER_CLANG)
                #define X_EXPORT __attribute__((visibility("default")))
                #define X_IMPORT __attribute__((visibility("default")))
                #define X_API __attribute__((visibility("default")))
                #define X_LOCAL __attribute__((visibility("hidden")))
            #else
                #define X_EXPORT
                #define X_IMPORT
                #define X_API
                #define X_LOCAL
            #endif
        #else
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

    // 类导出宏
    #define X_CLASS_EXPORT X_API

    // 模板导出宏（主要用于 Windows）
    #ifdef X_PLATFORM_WINDOWS
        #define X_TEMPLATE_EXPORT X_API
    #else
        #define X_TEMPLATE_EXPORT
    #endif
#endif

// ============================================================================
// 编译器特定宏
// ============================================================================

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

// ============================================================================
// 便利宏
// ============================================================================

// 变量声明宏
#define X_DECLARE_VARIABLE(type, name) X_API extern type name
#define X_DEFINE_VARIABLE(type, name, value) type name = value

// 函数导出宏
#define X_FUNCTION_EXPORT X_API

// 类导出宏（已定义，这里重复定义确保一致性）
#ifndef X_CLASS_EXPORT
    #define X_CLASS_EXPORT X_API
#endif

// ============================================================================
// 向后兼容的命名空间别名
// ============================================================================
// 注意：这个别名必须在包含此头文件的任何地方都可用
// 但由于命名空间定义可能在其他地方，我们使用条件编译
#ifdef XUtils
namespace xtd = XUtils;
#endif

#endif // X_VERSION_HPP
