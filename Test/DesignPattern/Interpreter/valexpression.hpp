#ifndef XUTILS2_VALEXPRESSION_HPP
#define XUTILS2_VALEXPRESSION_HPP

#include <expression.hpp>

class ValExpression : public ExpressionConstInt {
    int m_value_{};

public:
    constexpr explicit ValExpression(int const v)
    : m_value_(v) {}
    [[nodiscard]] int calculate() const noexcept override
    { return m_value_; }
};

#endif
