#ifndef XUTILS2_XINGNENGSTRATEGY_HPP
#define XUTILS2_XINGNENGSTRATEGY_HPP 1

#include <strategy.hpp>

class XingNengStrategy final : public Strategy{

public:
    std::string getResURL() override;
};

#endif
