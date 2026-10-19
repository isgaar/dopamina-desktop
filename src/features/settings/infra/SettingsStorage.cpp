/**
 * Location: src/features/settings/infra/SettingsStorage.cpp
 * Function: Implementation of settings persistence using QSettings for paths
 * and ICredentialStore for tokens ensuring strict security partitioning.
 * All Rights Reserved isgaar
 */
#include "SettingsStorage.h"
#include "core/ApiClient.h"
#include "core/ProjectConfig.h"
#include "core/credentials/ICredentialStore.h"
#include "core/platform/PlatformHelper.h"
#include "core/service/LocalServiceManager.h"
#include "shared/i18n/StringManager.h"
#include <QSettings>

namespace Dopamine::Features::Settings::Infra {

static bool s_testScope = false;
static QString organization() {
    return s_testScope ? QStringLiteral("isgaar_test") :
                         QStringLiteral("isgaar");
}

void SettingsStorage::setTestScope(bool enabled) { s_testScope = enabled; }

SettingsData SettingsStorage::load(
    const Core::Credentials::ICredentialStore *creds) {
    QSettings s(organization(), QStringLiteral("Dopamine"));
    SettingsData d;
    d.isFirstRun = !s.value(
        QStringLiteral("FirstRun/Completed"), false).toBool();
    d.mode = s.value(
        QStringLiteral("Source/Mode"), QStringLiteral("smb")).toString();
    d.serverUrl = s.value(
        QStringLiteral("Source/ServerUrl"),
        Core::ProjectConfig::instance().apiUrl()).toString();
    d.folderPath = s.value(
        QStringLiteral("Source/FolderPath"),
        Core::ProjectConfig::instance().localPath()).toString();
    d.smbHost = s.value(
        QStringLiteral("Source/SmbHost"),
        QStringLiteral("192.168.0.2")).toString();
    d.smbShare = s.value(
        QStringLiteral("Source/SmbShare"),
        QStringLiteral("datos")).toString();
    d.smbFolder = s.value(QStringLiteral("Source/SmbFolder")).toString();
    d.smbUser = s.value(
        QStringLiteral("Source/SmbUser"),
        QStringLiteral("ismael")).toString();
    d.saveCredentials = s.value(
        QStringLiteral("Source/SaveCredentials"), true).toBool();
    if (creds) {
        d.token = creds->retrieveToken(QStringLiteral("api"));
        d.smbPassword = creds->retrieveToken(QStringLiteral("smb"));
    }
    d.language = Shared::I18n::StringManager::instance().currentLocale();
    if (d.language.isEmpty()) d.language = QStringLiteral("system");
    return d;
}

void SettingsStorage::save(
    const SettingsData &data,
    Core::Credentials::ICredentialStore *creds) {
    QSettings s(organization(), QStringLiteral("Dopamine"));
    s.setValue(QStringLiteral("Source/Mode"), data.mode);
    s.setValue(QStringLiteral("Source/ServerUrl"), data.serverUrl);
    s.setValue(QStringLiteral("Source/FolderPath"), data.folderPath);
    s.setValue(QStringLiteral("Source/SmbHost"), data.smbHost);
    s.setValue(QStringLiteral("Source/SmbShare"), data.smbShare);
    s.setValue(QStringLiteral("Source/SmbFolder"), data.smbFolder);
    s.setValue(QStringLiteral("Source/SmbUser"), data.smbUser);
    s.setValue(QStringLiteral("Source/SaveCredentials"), data.saveCredentials);
    if (creds) {
        creds->storeToken(QStringLiteral("api"), data.token);
        if (data.saveCredentials) {
            creds->storeToken(QStringLiteral("smb"), data.smbPassword);
        } else {
            creds->deleteToken(QStringLiteral("smb"));
        }
    }
}

void SettingsStorage::apply(
    const SettingsData &data,
    Core::ApiClient *apiClient,
    Core::Service::LocalServiceManager *svc) {
    if (apiClient) {
        apiClient->setBearerToken(data.token);
    }
    auto &cfg = Core::ProjectConfig::instance();
    if (data.mode == QStringLiteral("local")) {
        cfg.setApiUrl(QStringLiteral("http://127.0.0.1:8000"));
        cfg.setLocalSource(data.folderPath);
        if (svc) svc->start(data.folderPath);
    } else if (data.mode == QStringLiteral("smb")) {
        cfg.setSmbSource(data.smbHost, data.smbShare,
                         data.smbFolder, data.smbUser);
        if (svc) svc->stop();
    } else {
        cfg.setApiUrl(data.serverUrl);
        if (svc) svc->stop();
    }
    auto &sm = Shared::I18n::StringManager::instance();
    auto &p = Core::Platform::PlatformHelper::instance();
    sm.setLocale(data.language == QStringLiteral("system")
                     ? p.detectHostLanguage() : data.language);
}

void SettingsStorage::setFirstRunCompleted() {
    QSettings s(organization(), QStringLiteral("Dopamine"));
    s.setValue(QStringLiteral("FirstRun/Completed"), true);
}

} // namespace Dopamine::Features::Settings::Infra
