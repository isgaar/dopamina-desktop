/**
 * Location: src/core/credentials/FileCredentialStore.cpp
 * Function: Implementation of the owner-only credential store. Each token
 * lives in a 0600 file so it never appears in QSettings or session XML.
 * All Rights Reserved isgaar
 */
#include "FileCredentialStore.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>

namespace Dopamine::Core::Credentials {

static QString defaultDirectory() {
    const QString base = QStandardPaths::writableLocation(
        QStandardPaths::GenericConfigLocation);
    return base + QStringLiteral("/isgaar/Dopamine/credentials");
}

static QString safeServiceName(const QString &service) {
    static const QRegularExpression valid(
        QStringLiteral("^[A-Za-z0-9._-]+$"));
    if (valid.match(service).hasMatch()) {
        return service;
    }
    return QString::fromLatin1(QCryptographicHash::hash(
        service.toUtf8(), QCryptographicHash::Sha256).toHex());
}

FileCredentialStore::FileCredentialStore(QString directory)
    : m_directory(directory.isEmpty() ? defaultDirectory() : directory) {}

QString FileCredentialStore::pathFor(const QString &service) const {
    return m_directory + QLatin1Char('/') +
        safeServiceName(service) + QStringLiteral(".token");
}

bool FileCredentialStore::storeToken(const QString &service,
                                     const QString &token) {
    if (service.isEmpty()) {
        return false;
    }
    if (token.isEmpty()) {
        return deleteToken(service);
    }
    if (!QDir().mkpath(m_directory)) {
        return false;
    }
    const QString path = pathFor(service);
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(token.toUtf8());
    if (!file.commit()) {
        return false;
    }
    QFile::setPermissions(path, QFileDevice::ReadOwner |
                                  QFileDevice::WriteOwner);
    return true;
}

QString FileCredentialStore::retrieveToken(const QString &service) const {
    QFile file(pathFor(service));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QString::fromUtf8(file.readAll()).trimmed();
}

bool FileCredentialStore::deleteToken(const QString &service) {
    const QString path = pathFor(service);
    return !QFile::exists(path) || QFile::remove(path);
}

} // namespace Dopamine::Core::Credentials
