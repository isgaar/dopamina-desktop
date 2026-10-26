/**
 * Location: src/core/ProjectConfig.h
 * Function: Centralized runtime configuration singleton providing API,
 * database path, streaming endpoints, and SMB/local music sources.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_PROJECT_CONFIG_H
#define DOPAMINE_CORE_PROJECT_CONFIG_H

#include <QObject>
#include <QString>

namespace Dopamine::Core {

class ProjectConfig : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString apiUrl READ apiUrl NOTIFY configChanged)
    Q_PROPERTY(bool isProduction READ isProduction NOTIFY configChanged)
    Q_PROPERTY(QString targetHost READ targetHost NOTIFY configChanged)
    Q_PROPERTY(int targetPort READ targetPort NOTIFY configChanged)
    Q_PROPERTY(QString sourceType READ sourceType NOTIFY configChanged)
    Q_PROPERTY(QString smbHost READ smbHost NOTIFY configChanged)
    Q_PROPERTY(QString smbShare READ smbShare NOTIFY configChanged)
    Q_PROPERTY(QString smbFolder READ smbFolder NOTIFY configChanged)
    Q_PROPERTY(QString smbUser READ smbUser NOTIFY configChanged)
    Q_PROPERTY(QString smbShareUrl READ smbShareUrl NOTIFY configChanged)
    Q_PROPERTY(QString localPath READ localPath NOTIFY configChanged)
    Q_PROPERTY(bool isSourceConfigured READ isSourceConfigured
               NOTIFY configChanged)

public:
    static ProjectConfig &instance();

    QString apiUrl() const { return m_apiUrl; }
    bool isProduction() const { return m_isProduction; }

    QString targetHost() const;
    int targetPort() const;
    void logActiveEndpoint() const;

    QString sourceType() const { return m_sourceType; }
    QString smbHost() const { return m_smbHost; }
    QString smbShare() const { return m_smbShare; }
    QString smbFolder() const { return m_smbFolder; }
    QString smbUser() const { return m_smbUser; }
    QString smbShareUrl() const;
    QString localPath() const { return m_localPath; }
    bool isSourceConfigured() const {
        return !m_smbHost.isEmpty() || !m_localPath.isEmpty();
    }

    void setApiUrl(const QString &url);
    void setServerSource();
    void setSmbSource(const QString &h, const QString &s,
                      const QString &f, const QString &u);
    void setLocalSource(const QString &path);
    void setSmbUrl(const QString &url);

signals:
    void configChanged();

private:
    explicit ProjectConfig(QObject *parent = nullptr);
    ~ProjectConfig() override = default;

    void loadEnvironment();

    QString m_apiUrl;
    bool m_isProduction = false;
    QString m_sourceType = QStringLiteral("smb");
    QString m_smbHost;
    QString m_smbShare;
    QString m_smbFolder;
    QString m_smbUser;
    QString m_localPath;
};

} // namespace Dopamine::Core

#endif // DOPAMINE_CORE_PROJECT_CONFIG_H
