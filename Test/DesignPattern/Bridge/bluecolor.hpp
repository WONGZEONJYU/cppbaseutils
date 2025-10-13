#ifndef BLUE_COLOR_HPP
#define BLUE_COLOR_HPP 1

#include <color.hpp>

class BlueColor final : public Color {
public:
    constexpr BlueColor() = default;
    ~BlueColor() override = default;
    [[nodiscard]] std::string color() const noexcept override;
};

#endif
