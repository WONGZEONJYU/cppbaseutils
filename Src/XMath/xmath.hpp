#ifndef XUTILS_XMATH_HPP
#define XUTILS_XMATH_HPP 1

#include <utility>
#include <XHelper/xversion.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

struct Range final {

    enum BoundType {
        Open,   // (a
        Closed  // [a
    };

    constexpr Range(double const min, double const max,
                    BoundType const left = Closed,
                    BoundType const right = Closed,
                    double const eps = 1e-9)
    :m_range_{min,max},m_eps_(eps),m_left(left),m_right(right) {}

    constexpr explicit Range(std::pair<double, double> const & range
                ,BoundType const left = Closed
                ,BoundType const right = Closed
                ,double const eps = 1e-9)
    :m_range_{range},m_eps_(eps),m_left(left),m_right(right) {}

    [[nodiscard]] constexpr bool contains(double const value) const noexcept {

        auto const leftOk  { Closed == m_left
                     ? value >= m_range_.first - m_eps_
                     : value >  m_range_.first + m_eps_ };

        auto const rightOk { Closed == m_right
                     ? value <= m_range_.second + m_eps_
                     : value <  m_range_.second - m_eps_ };

        return leftOk && rightOk;
    }

    constexpr bool operator()(double const value) const noexcept {
        return contains(value);
    }

private:
    std::pair<double,double> const m_range_{};
    double const m_eps_{};
    BoundType const m_left{},m_right{};
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
