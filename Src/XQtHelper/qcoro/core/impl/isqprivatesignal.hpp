#ifndef XUTILS2_IS_Q_PRIVATE_SIGNAL_HPP
#define XUTILS2_IS_Q_PRIVATE_SIGNAL_HPP 1

#pragma once

#include <XGlobal/xversion.hpp>
#include <string_view>
#include <type_traits>
#if defined(__cpp_lib_source_location)
#include <source_location>
#endif

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {
    /**
     * A whacky helper to detect whether T is a QPrivateSignal type.
     *
     * The problem with QPrivateSignal is that it's a struct that's private
     * to each QObject-derived class, so we can't simply do std::same_as_v<T, Obj::QPrivateSignal>
     * because we cannot access the private QPrivateSignal struct from here.
     *
     * The only solution I could come up with abuses std::source_location
     * to get a string with name of the function which also shows what T is,
     * and then check whether the string contains "::QPrivateSignal" substring
     * in the right place. The whole check is compile-time, so there's no runtime
     * overhead.
     *
     * Unfortunately the output format of std::source_location::function_name() is
     * implementation-specific, so we need to handle each compiler separately. This
     * can cause truble in the future if an implementation changes the output format.
     **/
    template<typename T>
    class is_QPrivateSignal {
        using type [[maybe_unused]] = T;
        static constexpr auto functionName() noexcept{
#if defined(_MSC_VER)
            // While MSVC does support std::source_location, std::source_location::function_name()
            // returns only the name of the function ("functionName"), but we need the fully qualified
            // name, which MSVC-specific __FUNCSIG__  macro gives us
            return __FUNCSIG__;
#elif defined(__cpp_lib_source_location)
            return std::source_location::current().function_name();
#else
            return __PRETTY_FUNCTION__;
#endif
        }

        static constexpr bool getValue() noexcept {
            // Clang: static auto QCoro::detail::is_QPrivateSignal<Foo>::functionName() [T = Foo]
            // GCC  : static consteval auto QCoro::detail::is_QPrivateSignal<T>::functionName() [with T = Foo]
            // MSVC : const char *__cdecl QCoro::detail::is_QPrivateSignal<Foo>::functionName(void)

            // Note: can't use auto here as it gets deduced as const char* for some reason despite
            // functionName() explicitly returning std::string_view.
            constexpr std::string_view name { functionName() };
#if defined(_MSC_VER)
            constexpr auto end_pos{ name.rfind('>') };
#else
            constexpr auto end_pos{ name.rfind(']') };
#endif
            constexpr std::string_view QPrivateSignal {"QPrivateSignal" };

            if (std::string_view::npos == end_pos || end_pos < QPrivateSignal.length())
            { return {}; }

            for (auto pos {1U}; pos <= QPrivateSignal.length(); ++pos) {
                if (QPrivateSignal[QPrivateSignal.length() - pos] != name[end_pos - pos])
                { return {}; }
            }
            return true;
        }

    public:
        static constexpr auto value{ getValue() };
    };

    template<typename T>
    inline constexpr auto is_QPrivateSignal_v { is_QPrivateSignal<std::remove_cvref_t<T>>::value };
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
