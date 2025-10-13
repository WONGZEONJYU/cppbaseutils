#ifndef ITERATOR_HPP
#define ITERATOR_HPP 1

#include <staff.hpp>

class Iterator {

protected:
    Iterator() = default;
public:
    virtual ~Iterator() = default;
    virtual Staff & first()  = 0;
    virtual Staff & next() = 0;
    [[nodiscard]] virtual bool isEnd() const = 0;
};

#endif
