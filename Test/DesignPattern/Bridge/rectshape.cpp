#include <rectshape.hpp>
#include <color.hpp>
#include <iostream>
#include <XHelper/xhelper.hpp>

void RectShape::draw() noexcept {
    std::cout << FUNC_SIGNATURE << "\t"
        << (this->color() ? this->color()->color() : "")
        << "\n";
}
