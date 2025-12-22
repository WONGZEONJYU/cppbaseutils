#ifndef XUTILS2_HAZARD_POINTER_HPP
#define XUTILS2_HAZARD_POINTER_HPP 1

#include <XAtomic/xatomic.hpp>

XUtils::XAtomicPointer<void> & get_hazard_pointer_for_current_thread();
bool outstanding_hazard_pointers_for(void * p) noexcept;

#endif
