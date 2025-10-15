#include <iostream>
#include <orderfacade.hpp>
#include <order.hpp>

int main() {
    constexpr OrderFacade orderFacade{};
    Order order{};
    if (auto const ret{ orderFacade.createOrder(order,"123456789") }
        ; ret != ErrorCode::OK)
    {}else {}

    return 0;
}
