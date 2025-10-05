#include <iostream>
#include <usdadapter.hpp>

int main() {
    constexpr Profit profit{};
    std::cout << profit.getProfit() << std::endl;

    constexpr USDProfit usd_profit{};
    std::cout << usd_profit.getUSDProfit({},{}) * 7.0  << std::endl;

    constexpr USDAdapter usd_adapter{};
    std::cout << usd_adapter.getProfit() << std::endl;

    return 0;
}
