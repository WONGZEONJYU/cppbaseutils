#include <usdadapter.hpp>

double USDAdapter::getProfit() const noexcept
{ return m_usd_profit_.getUSDProfit({},{}) * 7.0; }
