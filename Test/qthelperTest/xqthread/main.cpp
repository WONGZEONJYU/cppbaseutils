#include <QApplication>
#include <QPushButton>
#include <iostream>

template<typename ...Args>
constexpr void fff(Args && ...args)
{ std::invoke(std::forward<Args>(args)...); }

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QPushButton button {};
    button.setMinimumSize(400,300);
    button.show();

    return QApplication::exec();
}

