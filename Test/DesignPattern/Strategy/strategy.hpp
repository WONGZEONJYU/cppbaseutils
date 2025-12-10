#ifndef XUTILS2_STRATEGY_HPP
#define XUTILS2_STRATEGY_HPP 1

#include <string>

class Strategy {
public:
    virtual std::string getResURL() = 0;
    virtual ~Strategy() =default;
};

#endif