#include <XMemory/xmemory.hpp>
#include <coder.hpp>
#include <artist.hpp>
#include <visitor.hpp>

int main() {

    auto const coder1 { XUtils::makeShared<Coder>("coder1", 123)};
    auto const coder2 { XUtils::makeShared<Coder>("coder2", 432)};
    auto const coder3 { XUtils::makeShared<Coder>("coder3", 678)};
    auto const artist1 { XUtils::makeShared<Artist>("artist1", 5)};
    auto const artist2 { XUtils::makeShared<Artist>("artist2", 15)};

    std::vector<std::shared_ptr<Staff>> staffVec{};

    staffVec.push_back(coder1);
    staffVec.push_back(coder2);
    staffVec.push_back(coder3);
    staffVec.push_back(artist1);
    staffVec.push_back(artist2);

    Visitor visitor {};
    for (auto const & s : staffVec)
    { s->accept(std::addressof(visitor)); }

    return 0;
}

