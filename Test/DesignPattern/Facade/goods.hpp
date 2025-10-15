#ifndef XUTILS_GOODS_HPP
#define XUTILS_GOODS_HPP

#include <string>

struct Goods {
    std::string m_goodsNumber{},m_goodsName{};
    float m_goodsPrice{};
    int m_inventory{};
    Goods(std::string goodsNumber);
    [[nodiscard]] bool isValid() const noexcept;
};

#endif
