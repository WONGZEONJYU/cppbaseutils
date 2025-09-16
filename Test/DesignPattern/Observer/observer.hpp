#ifndef OBSERVER_HPP
#define OBSERVER_HPP 1

class Observer {
protected:
    constexpr Observer() = default;
public:
    virtual ~Observer() = default;
    virtual void update() = 0;
};

#endif
