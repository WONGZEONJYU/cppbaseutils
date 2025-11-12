#include <coder.hpp>
#include <visitor.hpp>

Coder::Coder(std::string name, int const codelines)
    :m_codelines_(codelines)
{ m_name_.swap(name); }

void Coder::accept(Visitor * const v)
{ v->visit(shared_from_this()); }
