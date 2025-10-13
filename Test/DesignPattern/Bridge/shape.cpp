#include <shape.hpp>

Color* Shape::color() const noexcept
{ return m_color_; }

void Shape::setColor(Color * const color) noexcept
{ m_color_ = color; }
