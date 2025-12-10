#ifndef XUTILS2_BUTTON_HPP
#define XUTILS2_BUTTON_HPP

#include <view.hpp>

class Button final : public View {
public:
    explicit Button();
    void action() override;
    void update() override;
};

#endif
