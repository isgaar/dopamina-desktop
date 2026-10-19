/**
 * Location: src/core/storage/SmbStorageProvider.cpp
 * Function: Implementation of SMB storage provider handling socket probing,
 * atomic server descriptor persistence, and KIO/GVFS mount point resolution.
 * All Rights Reserved isgaar
 */
#include "SmbStorageProvider.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QTcpSocket>
#include <unistd.h>

namespace Dopamine::Core::Storage {

SmbStorageProvider::SmbStorageProvider(
    const QString &h, const QString &s, const QString &f,
    const QString &u, const QString &mountOverride)
    : m_host(h), m_share(s), m_folder(f), m_user(u),
      m_mountOverride(mountOverride) {}

bool SmbStorageProvider::probe() {
    if (!m_mountOverride.isEmpty() && QDir(m_mountOverride).exists())
        return true;
    if (m_host.isEmpty()) {
        m_lastError = QStringLiteral("empty_host"); return false;
    }
    QTcpSocket sock; sock.connectToHost(m_host, 445);
    if (!sock.waitForConnected(1500)) {
        m_lastError = sock.errorString(); return false;
    }
    sock.disconnectFromHost(); return true;
}

bool SmbStorageProvider::connect() {
    if (!probe()) return false;
    m_connected = true; return true;
}

void SmbStorageProvider::disconnect() { m_connected = false; }

QString SmbStorageProvider::rootPath() const {
    if (!m_mountOverride.isEmpty() && QDir(m_mountOverride).exists()) {
        return m_mountOverride;
    }
    QString h = m_host;
    if (h.startsWith(QLatin1String("smb://"), Qt::CaseInsensitive))
        h = h.mid(6);
    int at = h.indexOf(QLatin1Char('@')); if (at != -1) h = h.mid(at + 1);
    int sl = h.indexOf(QLatin1Char('/')); if (sl != -1) h = h.left(sl);

    QString sub = m_share;
    while (sub.endsWith(QLatin1Char('/'))) sub.chop(1);
    if (!m_folder.isEmpty() && !sub.endsWith(m_folder)) {
        sub += QLatin1Char('/') + m_folder;
    }
    while (sub.endsWith(QLatin1Char('/'))) sub.chop(1);

    QString u = QStringLiteral("/run/user/%1").arg(getuid());
    for (const auto &k : QDir(u).entryList({QStringLiteral("kio-fuse*")},
                                           QDir::Dirs)) {
        QString cand = QStringLiteral("%1/%2/smb/%3/%4").arg(u, k, h, sub);
        if (QDir(cand).exists()) return cand;
    }
    return QStringLiteral("/run/user/%1/gvfs/smb-share:server=%2,share=%3")
        .arg(QString::number(getuid()), h, sub);
}

bool SmbStorageProvider::writeRootFile(
    const QString &fileName, const QByteArray &data) {
    QString p = rootPath();
    if (p.isEmpty()) {
        m_lastError = QStringLiteral("no_root_path"); return false;
    }
    QDir().mkpath(p);
    QSaveFile f(p + QLatin1Char('/') + fileName);
    if (!f.open(QIODevice::WriteOnly)) {
        m_lastError = f.errorString(); return false;
    }
    f.write(data); return f.commit();
}

QByteArray SmbStorageProvider::readRootFile(const QString &fileName) const {
    QFile f(rootPath() + QLatin1Char('/') + fileName);
    if (!f.open(QIODevice::ReadOnly)) {
        m_lastError = f.errorString(); return {};
    }
    return f.readAll();
}

bool SmbStorageProvider::existsRootFile(const QString &fileName) const {
    return QFile::exists(rootPath() + QLatin1Char('/') + fileName);
}

bool SmbStorageProvider::ensureServerDescriptor(
    const QString &serverId, const QString &libraryName,
    const QString &apiEndpoint) {
    if (existsRootFile(QStringLiteral(".dopamine_server.json"))) return true;
    QJsonObject obj;
    obj[QStringLiteral("server_id")] = serverId;
    obj[QStringLiteral("library_name")] = libraryName;
    obj[QStringLiteral("api_endpoint")] = apiEndpoint;
    obj[QStringLiteral("streaming_mode")] = QStringLiteral("http_range_206");
    obj[QStringLiteral("created_at")] =
        QString::number(QDateTime::currentSecsSinceEpoch());
    QJsonDocument doc(obj);
    return writeRootFile(
        QStringLiteral(".dopamine_server.json"), doc.toJson());
}

} // namespace Dopamine::Core::Storage
