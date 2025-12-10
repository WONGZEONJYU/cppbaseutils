#include <statuswaitexamine.hpp>
#include <XMemory/xmemory.hpp>
#include <statuspassed.hpp>
#include <statusfailed.hpp>

StatusPtr StatusWaitExamine::pass() {
    std::cout << FUNC_SIGNATURE << " : Passed!" << std::endl;
    return XUtils::makeShared<StatusPassed>();
}

StatusPtr StatusWaitExamine::fail() {
    std::cout << FUNC_SIGNATURE << " : Failed!" << std::endl;
    return XUtils::makeShared<StatusFailed>();
}
