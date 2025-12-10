#ifndef XUTILS2_EDITTEXT_HPP
#define XUTILS2_EDITTEXT_HPP

#include <view.hpp>

class EditText final : public View
{
public:
    explicit EditText();
    void action() override;
    void update() override;

};

#endif
