#ifndef XUTILS2_LISTVIEW_HPP
#define XUTILS2_LISTVIEW_HPP

#include <view.hpp>

class ListView final : public View
{
public:
    explicit ListView();
    void action() override;
    void update() override;
};

#endif
