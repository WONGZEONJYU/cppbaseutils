#ifndef XUTILS2_VIEW_HPP
#define XUTILS2_VIEW_HPP

enum ViewType : int {
    TEXTVIEW = 1
    ,LISTVIEW
    ,EDITTEXT
    ,BUTTON
};

class Medium;

class View {
    int m_type_{};
    Medium * m_medium_{};

public:
    virtual ~View() = default;
    int & rType() noexcept { return m_type_; }
    Medium * & rMedium() noexcept { return m_medium_; }
    virtual void action() = 0;
    virtual void update() = 0;
};

#endif
