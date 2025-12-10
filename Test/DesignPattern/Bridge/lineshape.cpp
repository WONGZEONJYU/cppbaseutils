#include <lineshape.hpp>
#include <iostream>
#include <color.hpp>
#include <XHelper/xhelper.hpp>

void LineShape::draw() noexcept {
    std::cout << FUNC_SIGNATURE << "\t"
        << (this->color() ? this->color()->color() : "") << "\n";
}
