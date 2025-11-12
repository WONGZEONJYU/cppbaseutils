#include <textview.hpp>
#include <iostream>
#include <medium.hpp>
#include <XHelper/xhelper.hpp>

TextView::TextView()
{ m_type_ = TEXTVIEW; }

void TextView::action() {
    std::cout << FUNC_SIGNATURE << std::endl;
    m_medium_->action(this);
}

void TextView::update()
{ std::cout << FUNC_SIGNATURE << std::endl; }
