#include <statuspassed.hpp>
#include <XHelper/xhelper.hpp>

StatusPtr StatusPassed::pass()
{ return Status::pass(); }

StatusPtr StatusPassed::fail()
{ return Status::fail(); }
