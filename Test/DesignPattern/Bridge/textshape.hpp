#ifndef TEXT_SHAPE_HPP
#define TEXT_SHAPE_HPP 1

#include <shape.hpp>

class TextShape final : public Shape {
public:
    constexpr TextShape() = default;
    ~TextShape() override = default;
    void draw() noexcept override;
};

#endif
