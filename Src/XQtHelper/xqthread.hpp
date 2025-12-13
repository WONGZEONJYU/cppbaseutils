#ifndef XUTILS2_X_QTHREAD_HPP
#define XUTILS2_X_QTHREAD_HPP 1

#include <XHelper/xqt_detection.hpp>
#include <XHelper/xcallablehelper.hpp>

#ifdef HAS_QT
#include <QThread>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XQThread : public QThread {


};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
#endif
