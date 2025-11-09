#ifndef XUTILS2_RUNNABLE_HPP
#define XUTILS2_RUNNABLE_HPP

class Runnable {
public:
    virtual void run() = 0;
    virtual ~Runnable() = default;
};

#endif
