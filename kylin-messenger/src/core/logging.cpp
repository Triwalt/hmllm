#include "core/logging.h"

#include <QCoreApplication>
#include "core/logging.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QDebug>
#include <QStandardPaths>
#include <QTextStream>
#include <QtGlobal>

#include <atomic>
#include <cstdlib>
#include <memory>
#include <mutex>

namespace {

std::unique_ptr<QFile> g_logFile;
QMutex g_logMutex;
std::atomic_bool g_initialized{false};

QString severityToString(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:
        return QStringLiteral("DEBUG");
    case QtInfoMsg:
        return QStringLiteral("INFO");
    case QtWarningMsg:
        return QStringLiteral("WARN");
    case QtCriticalMsg:
        return QStringLiteral("ERROR");
    case QtFatalMsg:
        return QStringLiteral("FATAL");
    }
    return QStringLiteral("UNKNOWN");
}

void writeLog(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    const QString timestamp = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    QString category = QString::fromLatin1(context.category ? context.category : "");
    if (category.isEmpty()) {
        category = QStringLiteral("kylin-messenger");
    }

    const QString formatted = QStringLiteral("[%1] [%2] [%3] %4 (%5:%6)\n")
                                  .arg(timestamp, severityToString(type), category, message,
                                      QString::fromLatin1(context.file ? context.file : "?"))
                                  .arg(context.line);

#ifndef NDEBUG
    QTextStream(stderr) << formatted;
#endif

    if (g_logFile && g_logFile->isOpen()) {
        QMutexLocker locker(&g_logMutex);
        QTextStream out(g_logFile.get());
        out << formatted;
        g_logFile->flush();
    }

    if (type == QtFatalMsg) {
        abort();
    }
}

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    writeLog(type, context, message);
}

void terminateHandler()
{
    writeLog(QtFatalMsg, QMessageLogContext(), QStringLiteral("std::terminate invoked"));
    std::_Exit(EXIT_FAILURE);
}

} // namespace

namespace KylinMessenger::Core::Logging {

void initialize(const QString& applicationId)
{
    bool expected = false;
    if (!g_initialized.compare_exchange_strong(expected, true)) {
        return;
    }

    QString appId = applicationId;
    if (appId.isEmpty()) {
        appId = QStringLiteral("kylin-messenger");
    }

    const QString logDirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(logDirPath);

    const QString logFilePath = QDir(logDirPath).filePath(appId + QStringLiteral(".log"));
    g_logFile = std::make_unique<QFile>(logFilePath);
    if (!g_logFile->open(QIODevice::Append | QIODevice::Text)) {
        QTextStream(stderr) << "Failed to open log file: " << logFilePath << "\n";
        g_logFile.reset();
    }

    qInstallMessageHandler(messageHandler);
    std::set_terminate(terminateHandler);

    qInfo() << "Logging initialized" << logFilePath;
}

void shutdown()
{
    if (!g_initialized.exchange(false)) {
        return;
    }

    qInstallMessageHandler(nullptr);
    std::set_terminate(nullptr);

    if (g_logFile) {
        QMutexLocker locker(&g_logMutex);
        g_logFile->flush();
        g_logFile->close();
        g_logFile.reset();
    }
}

} // namespace KylinMessenger::Core::Logging

