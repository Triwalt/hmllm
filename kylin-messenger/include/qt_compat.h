// qt_compat.h - helpers for Qt5/Qt6 compatibility
#pragma once
#include <QtGlobal>
#include <QDataStream>

// Define a macro for QDataStream version to avoid compile errors across Qt5/Qt6
#ifndef KYLIN_QDATASTREAM_VERSION
#  if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#    define KYLIN_QDATASTREAM_VERSION QDataStream::Qt_6_0
#  else
#    define KYLIN_QDATASTREAM_VERSION QDataStream::Qt_5_12
#  endif
#endif
