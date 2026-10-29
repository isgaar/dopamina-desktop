/**
 * Location: src/features/settings/app/SettingsController.cpp
 * Function: Settings controller implementation handling source modes, token
 * storage via ICredentialStore, persistence, and local service launch.
 * All Rights Reserved isgaar
 */
#include "SettingsController.h"
#include "../infra/SourceProbe.h"
#include "core/ApiClient.h"
#include "core/credentials/MemoryCredentialStore.h"
#include "core/service/LocalServiceManager.h"
#include "shared/colors/ColorManager.h"
#include <QDir>
#include <QSettings>
#include <QStandardPaths>

namespace Dopamine::Features::Settings::App {

SettingsController::SettingsController(
    Core::ApiClient *c, Core::Credentials::ICredentialStore *cr,
    Core::Service::LocalServiceManager *svc, QObject *p)
    : QObject(p), m_apiClient(c), m_creds(cr), m_service(svc) {
    if (!m_apiClient) { m_apiClient = new Core::ApiClient(this);
                        m_ownsClient = true; }
    if (!m_creds) { m_creds = new Core::Credentials::MemoryCredentialStore();
                    m_ownsCreds = true; }
    if (!m_service) { m_service = new Core::Service::LocalServiceManager(this);
                      m_ownsSvc = true; }
    resetDrafts();
    Infra::SettingsStorage::apply(m_saved, m_apiClient, m_service);
    if (!m_saved.isFirstRun) testSourceConnection();
}

SettingsController::~SettingsController() { if (m_ownsCreds) delete m_creds; }
QString SettingsController::qtVersion() const {
    return QString::fromUtf8(qVersion());
}

void SettingsController::resetDrafts() {
    m_draft = m_saved = Infra::SettingsStorage::load(m_creds);
    m_connectionStatus = m_sourceStatus = QStringLiteral("neutral");
    updateDirtyState();
    emit draftApiUrlChanged(); emit tokenChanged(); emit firstRunChanged();
    emit draftLanguageChanged(); emit draftThemeChanged();
    emit sourceDraftsChanged();
    emit connectionStatusChanged(); emit sourceStatusChanged();
}

#define IMPL_SET(fn, field, sig) \
    void SettingsController::fn(const QString &v) { \
        if (field != v) { field = v; emit sig(); updateDirtyState(); } \
    }
IMPL_SET(setDraftApiUrl, m_draft.serverUrl, draftApiUrlChanged)
IMPL_SET(setDraftToken, m_draft.token, tokenChanged)
IMPL_SET(setDraftLanguage, m_draft.language, draftLanguageChanged)
IMPL_SET(setDraftTheme, m_draft.theme, draftThemeChanged)
IMPL_SET(setDraftSourceType, m_draft.mode, sourceDraftsChanged)
IMPL_SET(setDraftLocalPath, m_draft.folderPath, sourceDraftsChanged)
IMPL_SET(setDraftSmbHost, m_draft.smbHost, sourceDraftsChanged)
IMPL_SET(setDraftSmbShare, m_draft.smbShare, sourceDraftsChanged)
IMPL_SET(setDraftSmbFolder, m_draft.smbFolder, sourceDraftsChanged)
IMPL_SET(setDraftSmbUser, m_draft.smbUser, sourceDraftsChanged)
IMPL_SET(setDraftSmbPassword, m_draft.smbPassword, sourceDraftsChanged)
#undef IMPL_SET

void SettingsController::setDraftSaveCredentials(bool s) {
    if (m_draft.saveCredentials != s) {
        m_draft.saveCredentials = s;
        emit sourceDraftsChanged();
        updateDirtyState();
    }
}

void SettingsController::updateDirtyState() {
    bool d = (m_draft != m_saved);
    if (m_dirty != d) { m_dirty = d; emit dirtyChanged(); }
}

void SettingsController::apply() {
    Infra::SettingsStorage::save(m_draft, m_creds);
    Infra::SettingsStorage::apply(m_draft, m_apiClient, m_service);
    m_saved = m_draft; updateDirtyState(); testSourceConnection();
}

void SettingsController::adoptRemoteServer(const QString &url) {
    QString clean = url.trimmed();
    while (clean.endsWith(QLatin1Char('/'))) clean.chop(1);
    if (clean.isEmpty()) return;
    m_draft.serverUrl = clean;
    m_draft.mode = QStringLiteral("server");
    m_draft.folderPath.clear();
    m_draft.isFirstRun = false;
    Infra::SettingsStorage::save(m_draft, m_creds);
    Infra::SettingsStorage::setFirstRunCompleted();
    Infra::SettingsStorage::apply(m_draft, m_apiClient, m_service);
    m_saved = m_draft;
    updateDirtyState();
    emit draftApiUrlChanged(); emit sourceDraftsChanged();
    emit firstRunChanged();
    testSourceConnection();
}

void SettingsController::discard() { resetDrafts(); }
void SettingsController::completeFirstRun() {
    Infra::SettingsStorage::setFirstRunCompleted();
    m_draft.isFirstRun = m_saved.isFirstRun = false;
    emit firstRunChanged(); apply();
}

void SettingsController::testConnection() {
    m_connectionStatus = QStringLiteral("checking");
    emit connectionStatusChanged();
    Infra::SourceProbe::probeApi(m_apiClient, m_draft.serverUrl,
        [this](bool ok) {
            m_connectionStatus = ok ? QStringLiteral("ok") :
                                      QStringLiteral("error");
            emit connectionStatusChanged();
        });
}

void SettingsController::testSourceConnection() {
    m_sourceStatus = QStringLiteral("checking");
    m_connectionStatus = QStringLiteral("connecting");
    emit sourceStatusChanged(); emit connectionStatusChanged();
    auto finish = [this](const QString &st) {
        m_sourceStatus = st;
        m_connectionStatus = (st == QStringLiteral("ok")) ?
            QStringLiteral("ok") : QStringLiteral("error");
        emit sourceStatusChanged(); emit connectionStatusChanged();
    };
    if (m_draft.mode == QStringLiteral("local")) {
        finish(Infra::SourceProbe::probeLocal(m_service, m_draft.folderPath));
        return;
    }
    if (m_draft.mode == QStringLiteral("smb")) {
        finish(Infra::SourceProbe::probeSmb(m_draft.smbHost)); return;
    }
    Infra::SourceProbe::probeApi(m_apiClient, m_draft.serverUrl,
        [finish](bool ok) { finish(ok ? QStringLiteral("ok") :
                                        QStringLiteral("error")); });
}

int SettingsController::sidebarWidth() const {
    QSettings s(QStringLiteral("isgaar"), QStringLiteral("Dopamine"));
    return s.value(QStringLiteral("Navigation/sidebarWidth"), 240).toInt();
}

void SettingsController::setSidebarWidth(int w) {
    if (w < 160 || w > 500) return;
    if (sidebarWidth() != w) {
        QSettings s(QStringLiteral("isgaar"), QStringLiteral("Dopamine"));
        s.setValue(QStringLiteral("Navigation/sidebarWidth"), w);
        emit sidebarWidthChanged();
    }
}

bool SettingsController::sidebarCollapsed() const {
    QSettings s(QStringLiteral("isgaar"), QStringLiteral("Dopamine"));
    return s.value(QStringLiteral("Navigation/sidebarCollapsed"),
                   false).toBool();
}

void SettingsController::setSidebarCollapsed(bool c) {
    if (sidebarCollapsed() != c) {
        QSettings s(QStringLiteral("isgaar"), QStringLiteral("Dopamine"));
        s.setValue(QStringLiteral("Navigation/sidebarCollapsed"), c);
        emit sidebarCollapsedChanged();
    }
}

QVariant SettingsController::getSetting(const QString &key, const QVariant &defaultValue) const {
    QSettings s(QStringLiteral("isgaar"), QStringLiteral("Dopamine"));
    return s.value(key, defaultValue);
}

void SettingsController::setSetting(const QString &key, const QVariant &value) {
    QSettings s(QStringLiteral("isgaar"), QStringLiteral("Dopamine"));
    s.setValue(key, value);
    if (key == QLatin1String("Appearance/dynamicAccent")) {
        Dopamine::Shared::Colors::ColorManager::instance().setDynamicAccentEnabled(value.toBool());
    }
}

void SettingsController::clearCoversCache() {
    const QString cacheBase = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir coversDir(cacheBase + QStringLiteral("/covers"));
    if (coversDir.exists()) {
        coversDir.removeRecursively();
    }
    emit coverCacheCleared();
}

void SettingsController::rescanLibrary() {
    emit rescanTriggered();
}

} // namespace Dopamine::Features::Settings::App
