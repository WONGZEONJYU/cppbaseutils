#ifndef XUTILS2_MEDIUM_HPP
#define XUTILS2_MEDIUM_HPP

#include <view.hpp>
#include <memory>
#include <vector>

class Medium {

    std::vector<std::shared_ptr<View>> views{};

public:
    constexpr Medium() = default;
    ~Medium() = default;
    void put(std::shared_ptr<View> const & view);
    void action(View * ) const;
};

#endif
