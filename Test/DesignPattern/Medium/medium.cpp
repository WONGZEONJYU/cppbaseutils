#include <medium.hpp>

void Medium::put(std::shared_ptr<View> const & view) {
    views.push_back(view);
    view->rMedium() = this;
}

void Medium::action(View * const v) const {
    if (BUTTON != v->rType() ) { return; }
    for (auto const & view : views) {
        if (EDITTEXT == view->rType() || LISTVIEW == view->rType())
        { view->update(); }
    }
}
