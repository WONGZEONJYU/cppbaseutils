#ifndef XUTILS_LIBTEST_HPP
#define XUTILS_LIBTEST_HPP

#include <XHelper/xhelper.hpp>

class LibTest;
LibTest * LibTestHandle();

class LibTest final : XUtils::XSingleton<LibTest>
{
    X_HELPER_CLASS
    int m_sss{100000};
    friend LibTest * LibTestHandle();
public:
    template<typename ...Args>
    inline static void print(Args && ...) {
        if (auto const d{instance()}) {
            d->pp();
        }
    }
private:
    LibTest() = default;
    ~LibTest() = default;
    bool construct_() const;
    void pp() const;
};

#endif
