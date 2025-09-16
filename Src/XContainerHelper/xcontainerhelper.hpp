#ifndef XUTILS_X_CONTAINER_HELPER_HPP
#define XUTILS_X_CONTAINER_HELPER_HPP 1

#include <XHelper/xhelper.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template <typename Con_>
auto sliced(Con_ const & c,std::size_t const s,std::size_t const e) noexcept -> Con_ {
    if (s >= c.size() ) { return {}; }
    auto const start { c.cbegin() + s };
    auto const subRang { std::ranges::subrange(start, start + std::ranges::min(e,c.size())) };
    return { subRang.cbegin() , subRang.cend() };
}

template<typename Con_,typename ...Args>
auto append(Con_ & c,Args && ...args) noexcept ->Con_ &
{ c.emplace_back(std::forward<decltype(args)>(args)...); return c; }

template<typename Con_,typename Con_R >
auto append(Con_ & fst ,Con_R const & snd) noexcept -> Con_ &
{ fst.insert(fst.cend(),snd.cbegin(),snd.cend()); return fst; }

template<typename Con_>
auto append(Con_ & c ,typename Con_::const_pointer const d,std::size_t const length) noexcept ->Con_ &
{ return append(c,std::ranges::subrange {d, d + length }); }

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
