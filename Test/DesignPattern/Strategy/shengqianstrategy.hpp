#ifndef XUTILS2_SHENGQIANSTRATEGY_HPP
#define XUTILS2_SHENGQIANSTRATEGY_HPP 1

#include <strategy.hpp>

class ShengQianStrategy final: public Strategy
{
public:
    std::string getResURL() override;
};

#endif
