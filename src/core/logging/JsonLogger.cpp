/**
 * Location: src/core/logging/JsonLogger.cpp
 * Function: Implementation of structured logging writing JSON-lines with
 * timestamp, redacted fields, rotation management, and stdout echoing.
 * All Rights Reserved isgaar
 */
#include "JsonLogger.h"
#include "core/errors/Redactor.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QJsonDocument>
#include <QStandardPaths>

namespace Core::Logging {

static QString levelToString(LogLevel l) {
    switch (l) {
    case LogLevel::Debug: return QStringLiteral("DEBUG");
    case LogLevel::Info:  return QStringLiteral("INFO");
    case LogLevel::Warn:  return QStringLiteral("WARN");
    case LogLevel::Error: return QStringLiteral("ERROR");
    }
    return QStringLiteral("INFO");
}

JsonLogger& JsonLogger::instance() {
    static JsonLogger s_instance;
    return s_instance;
}

JsonLogger::JsonLogger() {
    m_logDir = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation
    );
}

JsonLogger::~JsonLogger() {
    flush();
}

void JsonLogger::setLogDirectory(const QString& dirPath) {
    QMutexLocker locker(&m_mutex);
    m_logDir = dirPath;
}

void JsonLogger::rotateIfNeeded() {
    QString mainLog = m_logDir + QStringLiteral("/dopamine.log");
    QFileInfo fi(mainLog);
    if (!fi.exists() || fi.size() < m_maxBytes) return;

    for (int i = m_maxRotations - 1; i >= 1; --i) {
        QString oldPath = m_logDir + QStringLiteral("/dopamine.%1.log").arg(i);
        QString newPath = m_logDir
            + QStringLiteral("/dopamine.%1.log").arg(i + 1);
        if (QFile::exists(oldPath)) {

            QFile::remove(newPath);
            QFile::rename(oldPath, newPath);
        }
    }
    QString rot1 = m_logDir + QStringLiteral("/dopamine.1.log");
    QFile::remove(rot1);
    QFile::rename(mainLog, rot1);
}

void JsonLogger::log(
    LogLevel level,
    Core::Errors::ErrorCode code,
    const QString& component,
    const QString& operation,
    qint64 durMs,
    const QString& requestId,
    const QString& detail
) {
    QJsonObject obj;
    obj[QStringLiteral("ts")] = QDateTime::currentDateTimeUtc().toString(
        Qt::ISODateWithMs
    );
    obj[QStringLiteral("level")] = levelToString(level);
    obj[QStringLiteral("code")] = Core::Errors::errorCodeToString(code);
    obj[QStringLiteral("component")] = component;
    obj[QStringLiteral("operation")] = operation;
    obj[QStringLiteral("dur_ms")] = durMs;
    obj[QStringLiteral("request_id")] = requestId;
    obj[QStringLiteral("detail")] = Core::Errors::Redactor::redact(detail);

    QJsonDocument doc(obj);
    QByteArray line = doc.toJson(QJsonDocument::Compact) + "\n";

    QMutexLocker locker(&m_mutex);
    m_lastLine = QString::fromUtf8(line);

    if (!m_logDir.isEmpty()) {
        QDir().mkpath(m_logDir);
        rotateIfNeeded();
        QFile file(m_logDir + QStringLiteral("/dopamine.log"));
        if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
            file.write(line);
        }
    }
}

QString JsonLogger::lastLogLine() const {
    QMutexLocker locker(&m_mutex);
    return m_lastLine;
}

void JsonLogger::flush() {}

} // namespace Core::Logging
