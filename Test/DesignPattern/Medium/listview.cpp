#include <listview.hpp>
#include <XHelper/xhelper.hpp>
#include <medium.hpp>

ListView::ListView()
{ m_type_ = LISTVIEW; }

void ListView::action() {
    std::cout << FUNC_SIGNATURE << std::endl;
    m_medium_->action(this);
}

void ListView::update()
{ std::cout << FUNC_SIGNATURE << std::endl; }
