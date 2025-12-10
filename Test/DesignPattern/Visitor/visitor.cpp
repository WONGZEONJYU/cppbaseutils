#include <visitor.hpp>
#include <iostream>

void Visitor::visit(CoderPtr const & coder) const noexcept {
    (void)this;
    std::cout << "Coder name: " << coder->Name()
        << ", codelines: " << coder->codelines() << std::endl;
}

void Visitor::visit(ArtistPtr const & a) const noexcept {
    (void)this;
    std::cout << "Artist name: " << a->Name()
        << ", picNum: " << a->getPicNum() << std::endl;
}
