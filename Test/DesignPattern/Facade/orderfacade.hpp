#ifndef XUTILS_ORDER_FACADE_HPP
#define XUTILS_ORDER_FACADE_HPP 1

#include <string>

struct Order;

enum struct ErrorCode
{ OK,USER_ERROR,GOODS_ERROR };

class OrderFacade {
public:
    constexpr OrderFacade() = default;
    ErrorCode createOrder(Order & order
        , std::string const & goodsNumber)const noexcept;
};

#endif
