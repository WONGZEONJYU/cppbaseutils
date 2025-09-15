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

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
