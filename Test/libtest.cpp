#include "libtest.hpp"

bool LibTest::construct_() const {
    (void)this;
    return true;
}

void LibTest::pp() const {
    std::cerr <<  FUNC_SIGNATURE << " : " << m_sss << std::endl;
}

LibTest * LibTestHandle(){
    return LibTest::UniqueConstruction().get();
}
