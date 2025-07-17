#ifndef X_NAMESPACE_HPP
#define X_NAMESPACE_HPP 1

//XTD_VERSION "0.0.1"

#define XTD_VERSION_MAJOR 0

#define XTD_VERSION_MINOR 0

#define XTD_VERSION_PATCH 1

#define XTD_VERSION_CHECK(major, minor, patch) ((major<<16)|(minor<<8)|(patch))

#define XTD_VERSION XTD_VERSION_CHECK(XTD_VERSION_MAJOR,XTD_VERSION_MINOR,XTD_VERSION_PATCH)

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
