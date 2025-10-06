#include <lineshape.hpp>
#include <iostream>
#include <color.hpp>

void LineShape::draw() noexcept {
    std::cout << __PRETTY_FUNCTION__ << "\t"
        << (this->color() ? this->color()->color() : "") << "\n";
}
