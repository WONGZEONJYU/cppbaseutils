#ifndef COLOR_HPP
#define COLOR_HPP 1

#include <string>

class Color {
protected:
    constexpr Color() = default;

public:
    virtual ~Color() = default;
    [[nodiscard]] virtual std::string color() const = 0;
};

#endif
