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
#include "shared/colors/ColorManager.h"
#include "shared/i18n/StringManager.h"
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace Dopamine::Features::Settings::Infra {

static bool s_testScope = false;
static QString organization() {
    return s_testScope ? QStringLiteral("isgaar_test") :
                         QStringLiteral("isgaar");
}

static QString appearanceDir() {
    const QString base = QStandardPaths::writableLocation(
        QStandardPaths::GenericConfigLocation);
    return base + QLatin1Char('/') + organization() +
        QStringLiteral("/Dopamine");
}

QString SettingsStorage::appearanceFilePath() {
    return appearanceDir() + QStringLiteral("/appearance.xml");
}

static QString normalizedLanguage(const QString &language) {
    if (language == QStringLiteral("es") ||
        language == QStringLiteral("en")) {
        return language;
    }
    return QStringLiteral("system");
}

static QString normalizedTheme(const QString &theme) {
    const QString id = theme.trimmed().toLower();
    if (Shared::Colors::ColorManager::themeIds().contains(id)) {
        return id;
    }
    return QStringLiteral("roseglow");
}

struct AppearanceData {
    QString theme = QStringLiteral("roseglow");
    QString language = QStringLiteral("system");
};

static AppearanceData loadAppearance() {
    AppearanceData data;
    QFile file(SettingsStorage::appearanceFilePath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return data;
    }
    QXmlStreamReader reader(&file);
    while (!reader.atEnd()) {
        reader.readNext();
        if (!reader.isStartElement()) continue;
        if (reader.name() == QStringLiteral("theme")) {
            data.theme = normalizedTheme(reader.readElementText());
        } else if (reader.name() == QStringLiteral("language")) {
            data.language = normalizedLanguage(reader.readElementText());
        }
    }
    if (reader.hasError()) {
        return AppearanceData{};
    }
    return data;
}

static void saveAppearance(const QString &theme,
                           const QString &language) {
    QDir().mkpath(appearanceDir());
    QSaveFile file(SettingsStorage::appearanceFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }
    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement(QStringLiteral("appearance"));
    writer.writeTextElement(QStringLiteral("theme"),
                            normalizedTheme(theme));
    writer.writeTextElement(QStringLiteral("language"),
                            normalizedLanguage(language));
    writer.writeEndElement();
    writer.writeEndDocument();
    file.commit();
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
    const AppearanceData appearance = loadAppearance();
    d.language = appearance.language;
    d.theme = appearance.theme;
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
    saveAppearance(data.theme, data.language);
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
    Shared::Colors::ColorManager::instance().setTheme(data.theme);
    const QString language = normalizedLanguage(data.language);
    auto &sm = Shared::I18n::StringManager::instance();
    auto &p = Core::Platform::PlatformHelper::instance();
    sm.setLocale(language == QStringLiteral("system")
                     ? p.detectHostLanguage() : language);
}

void SettingsStorage::setFirstRunCompleted() {
    QSettings s(organization(), QStringLiteral("Dopamine"));
    s.setValue(QStringLiteral("FirstRun/Completed"), true);
}

} // namespace Dopamine::Features::Settings::Infra
