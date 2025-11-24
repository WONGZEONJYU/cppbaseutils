#ifndef XUTILS_XMATH_HPP
#define XUTILS_XMATH_HPP 1

#include <utility>
#include <XHelper/xversion.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

enum class BoundType {
    Open,   // (a
    Closed  // [a
};

template<typename = double> struct Range;

template<typename T>
struct Range final {

    static_assert(std::is_floating_point_v<T>,"T must be floating point!");

    using RangeType = std::pair<T,T>;

    constexpr Range(T const min, T const max,
                    BoundType const left = BoundType::Closed,
                    BoundType const right = BoundType::Closed,
                    T const eps = 1e-9)
    :m_range_{min,max},m_eps_(eps),m_left(left),m_right(right) {}

    constexpr explicit Range(RangeType const & range
                ,BoundType const left = BoundType::Closed
                ,BoundType const right = BoundType::Closed
                ,T const eps = 1e-9)
    :m_range_{range},m_eps_(eps),m_left(left),m_right(right) {}

    [[nodiscard]] constexpr bool contains(T const value) const noexcept {

        auto const leftOk{ BoundType::Closed == m_left
                     ? value >= m_range_.first - m_eps_
                     : value >  m_range_.first + m_eps_ };

        auto const rightOk{ BoundType::Closed == m_right
                     ? value <= m_range_.second + m_eps_
                     : value <  m_range_.second - m_eps_ };

        return leftOk && rightOk;
    }

    constexpr bool operator()(T const value) const noexcept
    { return contains(value); }

private:
    RangeType const m_range_{};
    T const m_eps_{};
    BoundType const m_left{},m_right{};
};

template<typename T>
Range(T, T) -> Range<T>;

template<typename T>
Range(std::pair<T, T>) -> Range<T>;

using RangeDouble = Range<>;
using RangeFloat = Range<float>;

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
