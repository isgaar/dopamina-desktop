/**
 * Location: src/core/logging/JsonLogger.h
 * Function: High-performance structured JSON-lines logger with thread-safe
 * file rotation and integrated security redaction for desktop diagnostics.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_LOGGING_JSONLOGGER_H
#define DOPAMINE_CORE_LOGGING_JSONLOGGER_H

#include <QMutex>
#include <QString>
#include "core/errors/ErrorCode.h"

namespace Core::Logging {

enum class LogLevel {
    Debug = 0,
    Info,
    Warn,
    Error
};

class JsonLogger {
public:
    static JsonLogger& instance();

    void setLogDirectory(const QString& dirPath);
    void log(
        LogLevel level,
        Core::Errors::ErrorCode code,
        const QString& component,
        const QString& operation,
        qint64 durMs = 0,
        const QString& requestId = QString(),
        const QString& detail = QString()
    );

    QString lastLogLine() const;
    void flush();

private:
    JsonLogger();
    ~JsonLogger();
    void rotateIfNeeded();

    mutable QMutex m_mutex;
    QString m_logDir;
    QString m_lastLine;
    qint64 m_maxBytes{2 * 1024 * 1024}; // 2 MiB
    int m_maxRotations{5};
};

} // namespace Core::Logging

#endif // DOPAMINE_CORE_LOGGING_JSONLOGGER_H
