#ifndef SHAPE_HPP
#define SHAPE_HPP 1

class Color;

class Shape {
    Color * m_color_{};
protected:
    constexpr Shape() = default;
    [[nodiscard]] Color * color() const noexcept;

public:
    virtual ~Shape() = default;
    virtual void draw() = 0;
    void setColor(Color * color) noexcept;
};

#endif
