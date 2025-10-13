#ifndef USD_ADAPTER_HPP
#define USD_ADAPTER_HPP 1

#include <profit.hpp>
#include <usdprofit.hpp>

class USDAdapter final : public Profit {
    USDProfit m_usd_profit_{};
public:
    constexpr USDAdapter() = default;
    [[nodiscard]] double getProfit() const noexcept override;
};

#endif
