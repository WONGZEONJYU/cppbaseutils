#include <edittext.hpp>
#include <medium.hpp>
#include <iostream>
#include <XHelper/xhelper.hpp>

EditText::EditText()
{ m_type_ = EDITTEXT; }

void EditText::action() {
    std::cout << FUNC_SIGNATURE << std::endl;
    m_medium_->action(this);
}

void EditText::update()
{ std::cout << FUNC_SIGNATURE << std::endl; }
