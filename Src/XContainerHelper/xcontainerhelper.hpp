#ifndef XUTILS_X_CONTAINER_HELPER_HPP
#define XUTILS_X_CONTAINER_HELPER_HPP 1

#include <XHelper/xhelper.hpp>
#include <charconv>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template <typename Con_>
requires std::ranges::range<Con_>
constexpr auto sliced(Con_ const & c,std::size_t const s,std::size_t const e) noexcept -> Con_ {
    if (s >= c.size() ) { return {}; }
    auto const start { c.cbegin() + s };
    auto const subRang { std::ranges::subrange(start, start + std::ranges::min(e,c.size())) };
    return { subRang.cbegin() , subRang.cend() };
}

#if 0
template<typename Con_>
constexpr auto append(Con_ & c,typename Con_::const_reference v) noexcept -> Con_ &
{ c.push_back(v); return c; }

template<typename Con_>
constexpr auto append(Con_ & c,typename Con_::value_type && v) noexcept -> Con_ &
{ c.push_back(std::forward<decltype(v)>(v)); return c; }
#endif

template<typename Con_,typename Con_R >
requires std::ranges::range<Con_> && std::ranges::input_range<Con_R>
constexpr auto append(Con_ & fst ,Con_R const & snd) noexcept -> Con_ &
{ fst.insert(fst.cend(),snd.cbegin(),snd.cend()); return fst; }

template<typename Con_>
requires std::ranges::range<Con_>
constexpr auto append(Con_ & c , typename Con_::const_pointer const d,std::size_t const length) noexcept -> Con_ &
{ return append(c,std::ranges::subrange{d, d + length } ); }

template<typename Con_,typename ...Args> requires std::is_constructible_v<Con_,Args...>
constexpr auto append(Con_ & c,Args && ...args) noexcept -> Con_ &
{ (c.push_back(std::forward<Args>(args)),...); return c; }

#if !defined(X_PLATFORM_MACOS)

template<typename T,typename STR>
requires std::is_arithmetic_v<T> && std::is_integral_v<T>
constexpr std::optional<T> toNum(STR && s,int const base = 10) noexcept {
    T value{};
    return std::from_chars(s.data(),s.data() + s.size(),value,base).ec == std::errc{}
    ? std::optional<T>{value} : std::nullopt;
}

template<typename T,typename STR>
requires std::is_arithmetic_v<T> && std::is_floating_point_v<T>
constexpr std::optional<T> toNum(STR && s,std::chars_format const fmt = std::chars_format::general) noexcept {
    T value{};
    return std::from_chars(s.data(),s.data() + s.size(),value,fmt).ec == std::errc{}
    ? std::optional<T>{value} : std::nullopt;
}

#endif

template<typename T,typename SS = std::stringstream,typename STR>
constexpr auto toNum(STR && s) -> std::optional<T>
{ SS ss {}; ss << s; T value{}; return ss >> value ? std::optional<T>{value} : std::nullopt; }

template<typename SS = std::stringstream,typename T>
constexpr auto toString(T && v,decltype(SS{}.precision()) const precision = SS{}.precision()) -> decltype(SS{}.str())
{ SS ss {};ss.precision(precision);ss << v;return ss.str(); }

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
