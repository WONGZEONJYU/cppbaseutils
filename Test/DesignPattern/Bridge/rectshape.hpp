#ifndef RECT_SHAPE_HPP
#define RECT_SHAPE_HPP 1

#include <shape.hpp>

class RectShape final : public Shape
{
public:
    constexpr RectShape() = default;
    ~RectShape() override = default;
    void draw() noexcept override;
};

#endif
