#ifndef USD_PROFIT_HPP
#define USD_PROFIT_HPP 1

#include <string>

class USDProfit {
public:
    constexpr USDProfit() = default;
    [[nodiscard]] double getUSDProfit(std::string const & app, std::string const & sec) const noexcept;
};

#endif
