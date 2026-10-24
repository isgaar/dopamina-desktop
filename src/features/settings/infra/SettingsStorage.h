/**
 * Location: src/features/settings/infra/SettingsStorage.h
 * Function: Persistent storage adapter managing QSettings preferences and
 * delegating bearer tokens to the secure ICredentialStore abstraction layer.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_SETTINGS_INFRA_SETTINGSSTORAGE_H
#define DOPAMINE_FEATURES_SETTINGS_INFRA_SETTINGSSTORAGE_H

#include <QString>

namespace Dopamine::Core {
class ApiClient;
namespace Credentials { class ICredentialStore; }
namespace Service { class LocalServiceManager; }
}

namespace Dopamine::Features::Settings::Infra {

struct SettingsData {
    QString mode;
    QString serverUrl;
    QString folderPath;
    QString token;
    QString language;
    QString theme;
    QString smbHost;
    QString smbShare;
    QString smbFolder;
    QString smbUser;
    QString smbPassword;
    bool saveCredentials = true;
    bool isFirstRun = false;

    bool operator==(const SettingsData &) const = default;
};

class SettingsStorage {
public:
    static SettingsData load(
        const Core::Credentials::ICredentialStore *creds);
    static void save(
        const SettingsData &data,
        Core::Credentials::ICredentialStore *creds);
    static void apply(
        const SettingsData &data,
        Core::ApiClient *apiClient,
        Core::Service::LocalServiceManager *svc);
    static void setFirstRunCompleted();
    static void setTestScope(bool enabled);
    static QString appearanceFilePath();
};

} // namespace Dopamine::Features::Settings::Infra

#endif // DOPAMINE_FEATURES_SETTINGS_INFRA_SETTINGSSTORAGE_H
