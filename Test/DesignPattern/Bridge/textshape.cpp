#include <textshape.hpp>
#include <color.hpp>
#include <iostream>
#include <XHelper/xhelper.hpp>

void TextShape::draw() noexcept {
    std::cout << FUNC_SIGNATURE << "\t"
        << (this->color() ? this->color()->color() : "") << "\n";
}
