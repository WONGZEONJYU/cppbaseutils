#include <QApplication>
#include <QPushButton>
#include <iostream>
#include <XQtHelper/qcoro/core/qcorocore.hpp>
#include <XQtHelper/qcoro/dbus/qcorodbus.hpp>
#include <XQtHelper/qcoro/network/qcoronetwork.hpp>
#include <XQtHelper/qcoro/qml/qcoroqml.hpp>
#include <XQtHelper/qcoro/quick/qcoroimageprovider.hpp>
#include <XQtHelper/qcoro/websockets/qcorowebsockets.hpp>
#include <XQtHelper/qcoro/test/qcorotest.hpp>

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

