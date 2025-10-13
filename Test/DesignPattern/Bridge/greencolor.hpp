#ifndef GREEN_COLOR_HPP
#define GREEN_COLOR_HPP 1

#include <color.hpp>

class GreenColor final : public Color {
public:
    constexpr GreenColor() = default;
    ~GreenColor() override = default;
    [[nodiscard]] std::string color() const noexcept override;
};

#endif
