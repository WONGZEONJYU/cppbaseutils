#ifndef X_NAMESPACE_HPP
#define X_NAMESPACE_HPP 1

#define XTD_VERSION "0.0.1"
#define XTD_NAMESPACE_BEGIN namespace xtd {
#define XTD_NAMESPACE_END }

#define XTD_INLINE_NAMESPACE_BEGIN(name) inline namespace name {
#define XTD_INLINE_NAMESPACE_END XTD_NAMESPACE_END

#ifdef WIN32
    #define DLLEXPORT dllexport
    #define DLLIMPORT dllimport
    #ifdef EXPORT_DLL
        #define DLLAPI DLLEXPORT
    #else
        #define DLLAPI DLLIMPORT
    #endif
#endif

#endif
