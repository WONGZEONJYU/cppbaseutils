#include "xhelper.hpp"
#include <iostream>
#include <algorithm>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

void x_assert(const char *expr, const char *file,const int &line) noexcept {
    std::cerr << "ASSERT:" << expr <<
        " in file " << file <<
            ", line " << line << "\n" << std::flush;
}

void x_assert(const std::string &expr, const std::string & file,const int &line) noexcept {
    std::cerr << "ASSERT: " << expr <<
        " in file " << file <<
            ", line " << line << "\n" << std::flush;
}

void x_assert(const std::string_view& expr, const std::string_view& file, const int& line) noexcept {
    std::cerr << "ASSERT: " << expr <<
        " in file " << file <<
            ", line " << line << "\n" << std::flush;
}

void x_assert_what(const char* where, const char* what,
    const char* file,const int &line) noexcept {
    std::cerr <<
        "ASSERT failure in " <<where <<
            ", what:" << what <<
                " in file " << file <<
                    ", line " << line << "\n" << std::flush;
}

void x_assert_what(const std::string& where, const std::string& what,
    const std::string& file,const int& line) noexcept {
    std::cerr <<
    "ASSERT failure in " <<where <<
        ", what:" << what <<
            " in file " << file <<
                ", line " << line << "\n" << std::flush;
}

void x_assert_what(const std::string_view &where, const std::string_view &what,
    const std::string_view &file,const int &line) noexcept {
    std::cerr <<
    "ASSERT failure in " << where <<
        ", what:" << what <<
            " in file " << file <<
                ", line " << line << "\n" << std::flush;
}

std::string toLower(std::string &str) {
#if __cplusplus >= 202002L
    std::ranges::transform(str,str.begin(),::tolower);
#else
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
#endif
    return str;
}

std::string toLower(std::string &&str) {
    return toLower(str);
}

std::string toLower(const std::string &str) {
    return toLower(std::string(str));
}

std::string toLower(const std::string_view &str) {
    return toLower(std::string(str));
}

std::string toLower(std::string_view &str) {
    return toLower(std::string(str));
}

std::string toLower(std::string_view &&str) {
    return toLower(std::string(str));
}

std::string toupper(std::string &str) {
    return toupper(std::move(str));
}

std::string toupper(std::string &&str) {
#if __cplusplus >= 202002L
    std::ranges::transform(str,str.begin(),::toupper);
#else
    std::transform(str.begin(),str.end(),str.begin(),::toupper);
#endif
    return str;
}

std::string toupper(const std::string &str) {
    return toupper(std::string(str));
}

std::string toupper(std::string_view &str) {
    return toupper(std::string(str));
}

std::string toupper(std::string_view &&str) {
    return toupper(std::string(str));
}

std::string toupper(const std::string_view &str) {
    return toupper(std::string(str));
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
