#ifndef XUTILS2_TEXTVIEW_HPP
#define XUTILS2_TEXTVIEW_HPP

#include <view.hpp>

class TextView final : public View {
public:
    explicit TextView();
    void action() override;
    void update() override;
};

#endif
