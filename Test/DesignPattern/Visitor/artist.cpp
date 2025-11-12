#include <artist.hpp>
#include <visitor.hpp>

Artist::Artist(std::string name, int const picNum)
    :m_picNum_(picNum)
{ m_name_.swap(name); }

void Artist::accept(Visitor * const v)
{ v->visit(shared_from_this()); }
