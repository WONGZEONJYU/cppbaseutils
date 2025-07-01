#include "xhelper.hpp"
#include <iostream>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

void x_assert(const char *expr, const char *file,const int &line) noexcept{
    std::cerr << "ASSERT:" << expr <<
        " in file " << file <<
            ", line " << line << "\n" << std::flush;
}

void x_assert(const std::string &expr, const std::string & file,const int &line) noexcept{
    std::cerr << "ASSERT: " << expr <<
        " in file " << file <<
            ", line " << line << "\n" << std::flush;
}

void x_assert(const std::string_view& expr, const std::string_view& file, const int& line) noexcept{
    std::cerr << "ASSERT: " << expr <<
        " in file " << file <<
            ", line " << line << "\n" << std::flush;
}

void x_assert_what(const char* where, const char* what,
    const char* file,const int &line) noexcept{
    std::cerr <<
        "ASSERT failure in " <<where <<
            ", what:" << what <<
                " in file " << file <<
                    ", line " << line << "\n" << std::flush;
}

void x_assert_what(const std::string& where, const std::string& what,
    const std::string& file,const int& line) noexcept{
    std::cerr <<
    "ASSERT failure in " <<where <<
        ", what:" << what <<
            " in file " << file <<
                ", line " << line << "\n" << std::flush;
}

void x_assert_what(const std::string_view &where, const std::string_view &what,
    const std::string_view &file,const int &line) noexcept{
    std::cerr <<
    "ASSERT failure in " << where <<
        ", what:" << what <<
            " in file " << file <<
                ", line " << line << "\n" << std::flush;
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
