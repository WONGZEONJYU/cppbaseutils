#ifndef RED_COLOR_HPP
#define RED_COLOR_HPP 1

#include <color.hpp>

class RedColor final: public Color {

public:
    constexpr RedColor() = default;
    ~RedColor() override = default;
    [[nodiscard]] std::string color() const noexcept override ;

};

#endif
