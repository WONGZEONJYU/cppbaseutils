#ifndef XUTILS2_EXPRESSION_HPP
#define XUTILS2_EXPRESSION_HPP

#include <XHelper/xhelper.hpp>

template<typename,typename >class Expression;

template<typename T>
class Expression<XUtils::Const,T> {
public:
    virtual ~Expression() = default;
    virtual T calculate() const = 0;
protected:
    constexpr Expression() = default;
};

template<typename T>
class Expression<XUtils::NonConst,T> {
public:
    virtual ~Expression() = default;
    virtual T calculate() = 0;
protected:
    constexpr Expression() = default;
};

using ExpressionInt = Expression<XUtils::NonConst,int>;
using ExpressionConstInt = Expression<XUtils::Const,int>;

#endif
