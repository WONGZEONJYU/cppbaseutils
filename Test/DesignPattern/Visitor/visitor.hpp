#ifndef XUTILS2_VISITOR_HPP
#define XUTILS2_VISITOR_HPP

#include <coder.hpp>
#include <artist.hpp>

class Visitor {

public:
    virtual ~Visitor() = default;
    constexpr Visitor() = default;
    void visit(CoderPtr const & ) const noexcept ;
    void visit(ArtistPtr const &) const noexcept;
};

#endif
