#ifndef XUTILS2_ADDEXPRESSION_HPP
#define XUTILS2_ADDEXPRESSION_HPP

#include <expression.hpp>
#include <memory>

using ExpressionConstIntPtr = std::shared_ptr<ExpressionConstInt>;

class AddExpression : public ExpressionConstInt {

     ExpressionConstIntPtr m_left_{},m_right_{};

public:
     explicit AddExpression(std::shared_ptr<ExpressionConstInt> const & left
          ,std::shared_ptr<ExpressionConstInt> const & right )
     : m_left_ {left},m_right_{right} {}

     [[nodiscard]] int calculate() const noexcept override
     { return m_left_->calculate() + m_right_->calculate(); }
};

#endif
