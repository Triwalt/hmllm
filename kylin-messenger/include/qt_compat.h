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

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#  define KYLIN_SPLIT_KEEP_EMPTY Qt::KeepEmptyParts
#  define KYLIN_SPLIT_SKIP_EMPTY Qt::SkipEmptyParts
#else
#  define KYLIN_SPLIT_KEEP_EMPTY QString::KeepEmptyParts
#  define KYLIN_SPLIT_SKIP_EMPTY QString::SkipEmptyParts
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
#  define KYLIN_QTCP_ERROR_SIGNAL &QTcpSocket::errorOccurred
#else
#  define KYLIN_QTCP_ERROR_SIGNAL static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error)
#endif
