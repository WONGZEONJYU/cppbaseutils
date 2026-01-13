#ifndef XUTILS_XMATH_HPP
#define XUTILS_XMATH_HPP 1

#include <utility>
#include <XGlobal/xversion.hpp>
#include <cstdint>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

enum class BoundType {
    Open,   // (a
    Closed  // [a
};

template<typename = double> struct Range;

template<typename T>
struct Range final {

    static_assert(std::is_arithmetic_v<T>,"T must be arithmetic!");

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

    constexpr bool leftCmp(T const value) const noexcept {
        if constexpr (std::is_floating_point_v<T>) {
            return BoundType::Closed == m_left
                 ? value >= m_range_.first - m_eps_
                 : value >  m_range_.first + m_eps_;
        }else {
            return BoundType::Closed == m_left
                 ? value >= m_range_.first
                 : value >  m_range_.first;
        }
    }

    constexpr bool rightCmp(T const value) const noexcept {
        if constexpr (std::is_floating_point_v<T>) {
            return BoundType::Closed == m_right
                 ? value <= m_range_.second + m_eps_
                 : value <  m_range_.second - m_eps_;
        }else {
            return BoundType::Closed == m_right
                 ? value <= m_range_.second
                 : value <  m_range_.second;
        }
    }

    [[nodiscard]] constexpr bool contains(T const value) const noexcept {
        auto const leftOk{ leftCmp(value) },rightOk{ rightCmp(value) };
        return leftOk && rightOk;
    }

    constexpr bool operator()(T const value) const noexcept
    { return contains(value); }

private:
    RangeType const m_range_{};
    T const m_eps_{};
    BoundType const m_left{},m_right{};
};

template<typename T> Range(T, T) -> Range<T>;
template<typename T> Range(std::pair<T, T>) -> Range<T>;

using RangeDouble = Range<>;
using RangeFloat = Range<float>;
using RangeChar = Range<char>;
using RangeSignedChar = Range<signed char>;
using RangeUnsignedChar = Range<unsigned char>;
using RangeShort = Range<short>;
using RangeUnsignedShort = Range<unsigned short>;
using RangeInt = Range<int>;
using RangeUnsignedInt = Range<unsigned int>;
using RangeLong = Range<long>;
using RangeUnsignedLong = Range<unsigned long>;
using RangeLongLong = Range<long long>;
using RangeUnsignedLongLong = Range<unsigned long long>;
using RangeInt64 = Range<int64_t>;
using RangeUInt64 = Range<uint64_t>;

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
