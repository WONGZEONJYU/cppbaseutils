#ifndef LINE_SHAPE_HPP
#define LINE_SHAPE_HPP 1

#include <shape.hpp>

class LineShape final : public Shape
{
public:
    constexpr LineShape() = default;
    ~LineShape() override = default;
    void draw() noexcept override ;
};

#endif
