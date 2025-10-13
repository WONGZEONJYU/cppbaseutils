#include <iostream>
#include <redcolor.hpp>
#include <lineshape.hpp>
#include <greencolor.hpp>
#include <rectshape.hpp>
#include <bluecolor.hpp>
#include <textshape.hpp>

int main() {

    {
        RedColor red{};
        LineShape line{};
        line.setColor(std::addressof(red));
        line.draw();
    }

    {
        GreenColor green{};
        RectShape rect{};
        rect.setColor(std::addressof(green));
        rect.draw();
    }

    {
        BlueColor blue{};
        TextShape text{};
        text.setColor(std::addressof(blue));
        text.draw();
    }

    return 0;
}
