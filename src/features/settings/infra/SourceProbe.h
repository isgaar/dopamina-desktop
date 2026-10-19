/**
 * Location: src/features/settings/infra/SourceProbe.h
 * Function: Diagnostics probe testing remote HTTP server endpoints and local
 * environment prerequisites such as Python and FFmpeg executable availability.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_SETTINGS_INFRA_SOURCEPROBE_H
#define DOPAMINE_FEATURES_SETTINGS_INFRA_SOURCEPROBE_H

#include <QString>
#include <functional>

namespace Dopamine::Core { class ApiClient; }
namespace Dopamine::Core::Service { class LocalServiceManager; }

namespace Dopamine::Features::Settings::Infra {

class SourceProbe {
public:
    static void probeApi(
        Core::ApiClient *client,
        const QString &url,
        std::function<void(bool ok)> callback);

    static QString probeLocal(
        const Core::Service::LocalServiceManager *svc,
        const QString &folder);

    static QString probeSmb(const QString &host, int port = 445);
};

} // namespace Dopamine::Features::Settings::Infra

#endif // DOPAMINE_FEATURES_SETTINGS_INFRA_SOURCEPROBE_H
