#include "xsignal_p.hpp"

extern "C" {
   using HandlerRoutine_ = int(*)(unsigned long);
   __declspec(dllimport) int __stdcall SetConsoleCtrlHandler(HandlerRoutine_,int);
}

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

int XSignalPrivate::HandlerRoutine(unsigned long const sig) {
   std::cerr << __FUNCTION__ << " signal" << sig << std::endl;
   for (auto && item : sm_signals | std::views::values) {
      item->m_sigEvent = sig;
      if (auto const f{ item->m_callable })
      { std::invoke(*f); }
   }
   return TRUE;
}

XSignalPrivate::XSignalPrivate() {
   SetConsoleCtrlHandler(HandlerRoutine,TRUE);
}

XSignalPrivate::~XSignalPrivate() {
   sm_signals.erase(m_key);
   if (sm_signals.empty()) { SetConsoleCtrlHandler(HandlerRoutine,FALSE); }
}

XSignal::XSignal()
   : m_d_ptr_ { std::make_unique<XSignalPrivate>() }
{ m_d_ptr_->m_x_ptr = this; }

XSignal::~XSignal() = default;

void XSignal::setHandlerHelper(std::string key,CallablePtr callable) noexcept {
   X_D(XSignal);
   d->m_key.swap(key);
   d->m_callable.swap(callable);
   XSignalPrivate::sm_signals[key] = d;
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
