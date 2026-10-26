/**
 * Location: src/core/ProjectConfig.cpp
 * Function: Centralized project configuration implementation loading
 * streaming endpoints, SMB storage sources, and local database paths.
 * All Rights Reserved isgaar
 */
#include "ProjectConfig.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcessEnvironment>
#include <QUrl>
#include <QDebug>

namespace Dopamine::Core {

ProjectConfig &ProjectConfig::instance() {
    static ProjectConfig s_instance;
    return s_instance;
}

ProjectConfig::ProjectConfig(QObject *parent) : QObject(parent) {
    loadEnvironment();
}

void ProjectConfig::loadEnvironment() {
    auto env = QProcessEnvironment::systemEnvironment();
    QString envMode = env.value("DOPAMINE_ENV", "local").toLower();
    m_isProduction = (envMode == "prod" || envMode == "production");

    QString defApi = m_isProduction
        ? "https://radio.dopamine.fm" : "http://127.0.0.1:8000";
    QString devHost = env.value("DOPAMINE_HOST", "");
    QString devPort = env.value("DOPAMINE_PORT", "8000");
    if (!m_isProduction && !devHost.isEmpty()) {
        defApi = QString("http://%1:%2").arg(devHost, devPort);
    }
    m_apiUrl = env.value("DOPAMINE_API_URL", defApi);

    QString smbUrl = env.value("DOPAMINE_SMB_URL", "");
    if (!smbUrl.isEmpty()) {
        setSmbUrl(smbUrl);
    }
    logActiveEndpoint();
}

void ProjectConfig::logActiveEndpoint() const {
    qInfo().noquote() << "[DopamineConfig] Modo:"
                      << (m_isProduction ? "PROD" : "DEV")
                      << "| API:" << m_apiUrl
                      << "| SMB:" << smbShareUrl();
}

QString ProjectConfig::targetHost() const {
    if (m_sourceType == QLatin1String("smb") && !m_smbHost.isEmpty()) {
        return m_smbHost;
    }
    QUrl u(m_apiUrl);
    return u.host().isEmpty() ? "127.0.0.1" : u.host();
}

int ProjectConfig::targetPort() const {
    QUrl u(m_apiUrl);
    return u.port() > 0 ? u.port() : 8000;
}

void ProjectConfig::setApiUrl(const QString &url) {
    if (m_apiUrl != url) { m_apiUrl = url; emit configChanged(); }
}

void ProjectConfig::setServerSource() {
    const bool changed = m_sourceType != QLatin1String("server") ||
        !m_smbHost.isEmpty() || !m_smbShare.isEmpty() ||
        !m_smbFolder.isEmpty() || !m_smbUser.isEmpty() ||
        !m_localPath.isEmpty();
    m_sourceType = QStringLiteral("server");
    m_smbHost.clear(); m_smbShare.clear();
    m_smbFolder.clear(); m_smbUser.clear();
    m_localPath.clear();
    if (changed) emit configChanged();
}

QString ProjectConfig::smbShareUrl() const {
    if (m_smbHost.isEmpty()) return {};
    QString u = m_smbUser.isEmpty() ? "" : m_smbUser + "@";
    QString f = m_smbFolder.isEmpty() ? "" : "/" + m_smbFolder;
    return QString("smb://%1%2/%3%4/").arg(u, m_smbHost, m_smbShare, f);
}

void ProjectConfig::setSmbSource(const QString &h, const QString &s,
                                 const QString &f, const QString &u) {
    m_smbHost = h; m_smbShare = s; m_smbFolder = f; m_smbUser = u;
    m_sourceType = QStringLiteral("smb");
    emit configChanged();
}

void ProjectConfig::setLocalSource(const QString &path) {
    m_localPath = path; m_sourceType = QStringLiteral("local");
    emit configChanged();
}

void ProjectConfig::setSmbUrl(const QString &url) {
    QUrl u(url);
    m_smbUser = u.userName(); m_smbHost = u.host();
    QString p = u.path();
    while (p.startsWith('/')) p.remove(0, 1);
    int slash = p.indexOf('/');
    if (slash != -1) {
        m_smbShare = p.left(slash);
        m_smbFolder = p.mid(slash + 1);
        while (m_smbFolder.endsWith('/')) m_smbFolder.chop(1);
    } else {
        m_smbShare = p; m_smbFolder.clear();
    }
    m_sourceType = QStringLiteral("smb");
    emit configChanged();
}

} // namespace Dopamine::Core
