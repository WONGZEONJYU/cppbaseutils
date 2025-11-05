#include <xhelper.hpp>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <XLog/xlog.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

void x_assert(const char *expr, const char *file,const int &line) noexcept {
    std::cerr << "ASSERT:" << expr
        << " in file " << file
        << ", line " << line << "\n" << std::flush;
    std::abort();
}

void x_assert(const std::string &expr, const std::string & file,const int &line) noexcept {
    std::cerr << "ASSERT: " << expr
        << " in file " << file
        << ", line " << line << "\n" << std::flush;
    std::abort();
}

void x_assert(const std::string_view& expr, const std::string_view& file, const int& line) noexcept {
    std::cerr << "ASSERT: " << expr
        << " in file " << file
        << ", line " << line << "\n" << std::flush;
    std::abort();
}

void x_assert_what(const char* where, const char* what,
    const char* file,const int &line) noexcept {
    std::cerr << "ASSERT failure in "
        << where << ", what:"<< what
        << " in file " << file
        << ", line " << line
        << "\n" << std::flush;
    std::abort();
}

void x_assert_what(const std::string& where, const std::string& what,
    const std::string& file,const int& line) noexcept {
    std::cerr <<
        "ASSERT failure in " << where
        << ", what:" << what
        << " in file " << file
        << ", line " << line
        << "\n" << std::flush;
    std::abort();
}

void x_assert_what(const std::string_view &where, const std::string_view &what,
    const std::string_view &file,const int &line) noexcept {
    std::cerr <<
        "ASSERT failure in " << where
        << ", what:" << what
        << " in file " << file
        << ", line " << line
        << "\n" << std::flush;
    std::abort();
}

void consoleOut(std::string const & s) noexcept
{ XLog::consoleOut(s); XLOG_FATAL(s); }

[[maybe_unused]] std::string toLower(std::string &str) {
#if __cplusplus >= 202002L
    std::ranges::transform(str,str.begin(),[](uint8_t const ch){return static_cast<char>(std::tolower(ch));});
#else
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
#endif
    return str;
}

#if 0
[[maybe_unused]]std::string toLower(std::string &&str)
{ return toLower(str); }

[[maybe_unused]]std::string toLower(const std::string &str)
{ return toLower(std::string(str)); }

[[maybe_unused]]std::string toLower(const std::string_view &str)
{ return toLower(std::string(str)); }

[[maybe_unused]]std::string toLower(std::string_view &str)
{ return toLower(std::string(str)); }

[[maybe_unused]]std::string toLower(std::string_view &&str)
{ return toLower(std::string(str)); }

[[maybe_unused]] std::string toUpper(std::string &str)
{ return toUpper(std::move(str)); }

[[maybe_unused]] std::string toUpper(std::string &&str) {
#if __cplusplus >= 202002L
    std::ranges::transform(str,str.begin(),[](uint8_t const ch){return static_cast<char>(std::toupper(ch));});
#else
    std::transform(str.begin(),str.end(),str.begin(),::toupper);
#endif
    return str;
}

[[maybe_unused]] std::string toUpper(const std::string &str)
{ return toUpper(std::string(str)); }

[[maybe_unused]] std::string toUpper(std::string_view &str)
{ return toUpper(std::string(str)); }

[[maybe_unused]] std::string toUpper(std::string_view &&str)
{ return toUpper(std::string(str)); }

[[maybe_unused]] std::string toUpper(const std::string_view &str)
{ return toUpper(std::string(str)); }

#endif


void XUtilsLibErrorLog::log(std::string_view const & msg)
{
    constexpr std::string_view logName {"XUtils2Lib.log"};

    constexpr auto fileMaxSize{1024 * 1024 * 1024};

    using namespace std::filesystem;

    try {
        if (exists(logName)) {
            if (auto const fileSize{file_size(logName)}
            ;fileSize >= fileMaxSize)
            { remove(logName); }
        }
    } catch (filesystem_error const & e) {
        std::cerr << FUNC_SIGNATURE << " " << e.what() << "\n";
        return;
    }

    try {
        std::ofstream logFs {logName.data(),std::ios::app};

        if (!logFs) {
            std::cerr << FUNC_SIGNATURE << " : " << logName << " open failed!\n";
            return;
        }

        logFs << ( std::ostringstream {} << XLog::getCurrentTimestamp()
            << " XUtilsLib error message: " << msg ).str() << "\n";

        logFs.flush();

    } catch (std::exception const &e) {
        std::cerr <<  logName << " Write err! " << e.what() << '\n';
    }
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
