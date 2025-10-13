#ifndef PROFIT_HPP
#define PROFIT_HPP 1

class Profit {

public:
    constexpr Profit() = default;
    virtual ~Profit() = default;
    [[nodiscard]] virtual double getProfit() const noexcept;
};

#endif
