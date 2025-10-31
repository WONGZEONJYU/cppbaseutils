#ifndef XUTILS_ORDER_HPP
#define XUTILS_ORDER_HPP

#include <string>

struct Order {
    std::string m_orderNumber{},m_goodsNumber{};
    float m_orderPrice{};
};

#endif
