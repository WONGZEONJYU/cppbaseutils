#ifndef COMPANY_A_HPP
#define COMPANY_A_HPP 1

#include <director.hpp>

class CompanyA final: public Director {
public:
    SystemConfig & buildSystemConfig() override;
};

#endif
