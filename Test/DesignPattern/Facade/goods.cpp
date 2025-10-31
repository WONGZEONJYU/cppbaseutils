#include <goods.hpp>

Goods::Goods(std::string goodsNumber)
    :m_goodsNumber{std::move(goodsNumber)}
    ,m_goodsName {"xxx"}
    ,m_goodsPrice {50.0}
    ,m_inventory {100}
{}

bool Goods::isValid() const noexcept { return true; }
