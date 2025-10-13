#ifndef COMPANY_B_HPP
#define COMPANY_B_HPP 1

#include <director.hpp>

class CompanyB final: public Director {
public:
    SystemConfig & buildSystemConfig() override;
};

#endif
