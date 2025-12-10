#include <status.hpp>
#include <XHelper/xhelper.hpp>

StatusPtr Status::pass() {
    std::cout << FUNC_SIGNATURE << " : The current operation is not supported" << std::endl;
    return {};
}

StatusPtr Status::fail() {
    std::cout << FUNC_SIGNATURE << " : The current operation is not supported" << std::endl;
    return {};
}
