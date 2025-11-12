#include <medium.hpp>
#include <textview.hpp>
#include <listview.hpp>
#include <edittext.hpp>
#include <button.hpp>
#include <XMemory/xmemory.hpp>

int main() {

    Medium medium{};

    auto const textview { XUtils::makeShared<TextView>() };
    auto const listview { XUtils::makeShared<ListView>() };
    auto const edittext { XUtils::makeShared<EditText>() };
    auto const button { XUtils::makeShared<Button>() };

    medium.put(textview);
    medium.put(listview);
    medium.put(edittext);
    medium.put(button);

    button->action();

    return 0;
}
