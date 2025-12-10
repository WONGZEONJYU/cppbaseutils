#include <button.hpp>
#include <medium.hpp>
#include <iostream>
#include <XHelper/xhelper.hpp>

Button::Button() { m_type_ = BUTTON; }

void Button::action() {
    std::cout << FUNC_SIGNATURE << std::endl;
    m_medium_->action(this);
}

void Button::update()
{ std::cout << FUNC_SIGNATURE << std::endl; }
