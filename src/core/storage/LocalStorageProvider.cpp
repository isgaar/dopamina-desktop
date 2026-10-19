/**
 * Location: src/core/storage/LocalStorageProvider.cpp
 * Function: Local filesystem storage provider implementation providing
 * directory validation, file reading/writing, and descriptor management.
 * All Rights Reserved isgaar
 */
#include "LocalStorageProvider.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

namespace Dopamine::Core::Storage {

LocalStorageProvider::LocalStorageProvider(const QString &folderPath)
    : m_folderPath(folderPath) {}

bool LocalStorageProvider::probe() {
    if (m_folderPath.isEmpty() || !QDir(m_folderPath).exists()) {
        m_lastError = QStringLiteral("folder_not_found");
        return false;
    }
    return true;
}

bool LocalStorageProvider::connect() {
    if (!probe()) return false;
    m_connected = true;
    return true;
}

void LocalStorageProvider::disconnect() { m_connected = false; }

bool LocalStorageProvider::writeRootFile(
    const QString &fileName, const QByteArray &data) {
    if (!probe()) return false;
    QSaveFile f(m_folderPath + QLatin1Char('/') + fileName);
    if (!f.open(QIODevice::WriteOnly)) {
        m_lastError = f.errorString();
        return false;
    }
    f.write(data);
    return f.commit();
}

QByteArray LocalStorageProvider::readRootFile(const QString &fileName) const {
    QFile f(m_folderPath + QLatin1Char('/') + fileName);
    if (!f.open(QIODevice::ReadOnly)) {
        m_lastError = f.errorString();
        return {};
    }
    return f.readAll();
}

bool LocalStorageProvider::existsRootFile(const QString &fileName) const {
    return QFile::exists(m_folderPath + QLatin1Char('/') + fileName);
}

bool LocalStorageProvider::ensureServerDescriptor(
    const QString &serverId, const QString &libraryName,
    const QString &apiEndpoint) {
    if (existsRootFile(QStringLiteral(".dopamine_server.json"))) {
        return true;
    }
    QJsonObject obj;
    obj[QStringLiteral("server_id")] = serverId;
    obj[QStringLiteral("library_name")] = libraryName;
    obj[QStringLiteral("api_endpoint")] = apiEndpoint;
    obj[QStringLiteral("streaming_mode")] =
        QStringLiteral("http_range_206");
    obj[QStringLiteral("created_at")] =
        QString::number(QDateTime::currentSecsSinceEpoch());
    QJsonDocument doc(obj);
    return writeRootFile(
        QStringLiteral(".dopamine_server.json"), doc.toJson());
}

} // namespace Dopamine::Core::Storage
