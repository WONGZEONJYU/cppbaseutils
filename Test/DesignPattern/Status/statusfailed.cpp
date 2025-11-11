#include <statusfailed.hpp>

StatusPtr StatusFailed::pass()
{ return Status::pass(); }

StatusPtr StatusFailed::fail()
{ return Status::fail(); }
