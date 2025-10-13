#ifndef DIRECTOR_HPP
#define DIRECTOR_HPP 1

#include <systemconfig.hpp>
#include <systemconfigbuilder.hpp>

class Director {
protected:
    constexpr Director() = default;
    SystemConfigBuilder builder{};
public:
    virtual ~Director() = default;
    virtual SystemConfig & buildSystemConfig() = 0;
};

#endif
