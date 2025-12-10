#ifndef XUTILS2_RESMANAGER_HPP
#define XUTILS2_RESMANAGER_HPP 1

#include <string>

class Strategy;

class ResManager {
public:
    std::string getRes(Strategy * s, const std::string & id) const;
};

#endif
