#include <orderfacade.hpp>
#include <user.hpp>
#include <goods.hpp>
#include <order.hpp>

ErrorCode OrderFacade::createOrder(Order & order, std::string const& goodsNumber) const noexcept{
    constexpr User user{};

    if (!user.isLogin()) { return ErrorCode::USER_ERROR; }

    if (!user.isLegal()) { return ErrorCode::USER_ERROR; }

    Goods goods(goodsNumber);

    if (!goods.isValid()) { return ErrorCode::GOODS_ERROR; }

    if (goods.m_inventory <= 0) { return ErrorCode::GOODS_ERROR; }

    order.m_orderNumber = "987654321";
    order.m_goodsNumber = goods.m_goodsNumber;
    order.m_orderPrice = goods.m_goodsPrice;
    --goods.m_inventory;
    return ErrorCode::OK;
}
