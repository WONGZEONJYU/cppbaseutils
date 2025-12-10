#ifndef XUTILS2_STATUS_HPP
#define XUTILS2_STATUS_HPP

#include <memory>

class Status;
using StatusPtr = std::shared_ptr<Status>;

class Status {

public:
    virtual ~Status() = default;
    virtual StatusPtr pass();
    virtual StatusPtr fail();

protected:
    constexpr Status() = default;
};

#endif
