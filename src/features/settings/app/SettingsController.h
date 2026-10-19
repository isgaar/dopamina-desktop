/**
 * Location: src/features/settings/app/SettingsController.h
 * Function: Settings controller managing draft parameters, dirty tracking,
 * server and local folder sources, credentials, and transactional commits.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_SETTINGS_APP_SETTINGSCONTROLLER_H
#define DOPAMINE_FEATURES_SETTINGS_APP_SETTINGSCONTROLLER_H

#include "../infra/SettingsStorage.h"
#include <QObject>
#include <QString>

namespace Dopamine::Core {
class ApiClient;
namespace Credentials { class ICredentialStore; }
namespace Service { class LocalServiceManager; }
}

namespace Dopamine::Features::Settings::App {

class SettingsController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString draftApiUrl READ draftApiUrl WRITE setDraftApiUrl
               NOTIFY draftApiUrlChanged)
    Q_PROPERTY(QString draftToken READ draftToken WRITE setDraftToken
               NOTIFY tokenChanged)
    Q_PROPERTY(QString draftLanguage READ draftLanguage WRITE setDraftLanguage
               NOTIFY draftLanguageChanged)
    Q_PROPERTY(QString draftSourceType READ draftSourceType
               WRITE setDraftSourceType NOTIFY sourceDraftsChanged)
    Q_PROPERTY(QString draftLocalPath READ draftLocalPath
               WRITE setDraftLocalPath NOTIFY sourceDraftsChanged)
    Q_PROPERTY(QString draftSmbHost READ draftSmbHost
               WRITE setDraftSmbHost NOTIFY sourceDraftsChanged)
    Q_PROPERTY(QString draftSmbShare READ draftSmbShare
               WRITE setDraftSmbShare NOTIFY sourceDraftsChanged)
    Q_PROPERTY(QString draftSmbFolder READ draftSmbFolder
               WRITE setDraftSmbFolder NOTIFY sourceDraftsChanged)
    Q_PROPERTY(QString draftSmbUser READ draftSmbUser
               WRITE setDraftSmbUser NOTIFY sourceDraftsChanged)
    Q_PROPERTY(QString draftSmbPassword READ draftSmbPassword
               WRITE setDraftSmbPassword NOTIFY sourceDraftsChanged)
    Q_PROPERTY(bool draftSaveCredentials READ draftSaveCredentials
               WRITE setDraftSaveCredentials NOTIFY sourceDraftsChanged)
    Q_PROPERTY(bool dirty READ isDirty NOTIFY dirtyChanged)
    Q_PROPERTY(bool isFirstRun READ isFirstRun NOTIFY firstRunChanged)
    Q_PROPERTY(QString connectionStatus READ connectionStatus
               NOTIFY connectionStatusChanged)
    Q_PROPERTY(QString sourceStatus READ sourceStatus
               NOTIFY sourceStatusChanged)
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString qtVersion READ qtVersion CONSTANT)

public:
    explicit SettingsController(
        Core::ApiClient *c = nullptr,
        Core::Credentials::ICredentialStore *cr = nullptr,
        Core::Service::LocalServiceManager *s = nullptr,
        QObject *p = nullptr);
    ~SettingsController() override;

    QString draftApiUrl() const { return m_draft.serverUrl; }
    void setDraftApiUrl(const QString &u);
    QString draftToken() const { return m_draft.token; }
    void setDraftToken(const QString &t);
    QString draftLanguage() const { return m_draft.language; }
    void setDraftLanguage(const QString &l);
    QString draftSourceType() const { return m_draft.mode; }
    void setDraftSourceType(const QString &t);
    QString draftLocalPath() const { return m_draft.folderPath; }
    void setDraftLocalPath(const QString &p);
    QString draftSmbHost() const { return m_draft.smbHost; }
    void setDraftSmbHost(const QString &h);
    QString draftSmbShare() const { return m_draft.smbShare; }
    void setDraftSmbShare(const QString &s);
    QString draftSmbFolder() const { return m_draft.smbFolder; }
    void setDraftSmbFolder(const QString &f);
    QString draftSmbUser() const { return m_draft.smbUser; }
    void setDraftSmbUser(const QString &u);
    QString draftSmbPassword() const { return m_draft.smbPassword; }
    void setDraftSmbPassword(const QString &p);
    bool draftSaveCredentials() const { return m_draft.saveCredentials; }
    void setDraftSaveCredentials(bool s);

    bool isDirty() const { return m_dirty; }
    bool isFirstRun() const { return m_draft.isFirstRun; }
    QString connectionStatus() const { return m_connectionStatus; }
    QString sourceStatus() const { return m_sourceStatus; }
    QString version() const { return QStringLiteral("2.0.0"); }
    QString qtVersion() const;

    Q_INVOKABLE void apply(); Q_INVOKABLE void discard();
    Q_INVOKABLE void resetDrafts(); Q_INVOKABLE void completeFirstRun();
    Q_INVOKABLE void testConnection();
    Q_INVOKABLE void testSourceConnection();

signals:
    void draftApiUrlChanged(); void tokenChanged();
    void draftLanguageChanged(); void sourceDraftsChanged();
    void dirtyChanged(); void firstRunChanged();
    void connectionStatusChanged(); void sourceStatusChanged();
private:
    void updateDirtyState();

    Core::ApiClient *m_apiClient = nullptr;
    Core::Credentials::ICredentialStore *m_creds = nullptr;
    Core::Service::LocalServiceManager *m_service = nullptr;
    bool m_ownsClient = false, m_ownsCreds = false, m_ownsSvc = false;

    Infra::SettingsData m_saved, m_draft;
    bool m_dirty = false;
    QString m_connectionStatus = QStringLiteral("neutral");
    QString m_sourceStatus = QStringLiteral("neutral");
};

} // namespace Dopamine::Features::Settings::App

#endif // DOPAMINE_FEATURES_SETTINGS_APP_SETTINGSCONTROLLER_H
