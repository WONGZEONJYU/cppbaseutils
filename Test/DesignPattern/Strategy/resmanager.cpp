#include <resmanager.hpp>
#include <strategy.hpp>

std::string ResManager::getRes(Strategy * const s, const std::string& id) const {
    (void)this;
    return s->getResURL() + '/' + id;
}
