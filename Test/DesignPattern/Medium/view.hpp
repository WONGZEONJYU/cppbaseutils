#ifndef XUTILS2_VIEW_HPP
#define XUTILS2_VIEW_HPP

#include <memory>

enum ViewType : int {
    TEXTVIEW = 1
    ,LISTVIEW
    ,EDITTEXT
    ,BUTTON
};

class Medium;
class View;
using ViewPtr = std::shared_ptr<View>;

class View {

protected:
    int m_type_{};
    Medium * m_medium_{};

public:
    virtual ~View() = default;
    int & rType() noexcept { return m_type_; }
    Medium * & rMedium() noexcept { return m_medium_; }
    virtual void action() = 0;
    virtual void update() = 0;

protected:
    constexpr View() = default;
};

#endif
