/**
 * Location: src/core/platform/media/PlatformMediaService.cpp
 * Function: Implementation of desktop platform media coordinator instantiating
 * host subsystem integrations (MPRIS 2 D-Bus on Linux, OS media relays).
 * All Rights Reserved isgaar
 */
#include "PlatformMediaService.h"

#if defined(DOPAMINE_HAS_DBUS)
#include "mpris/MprisPlayerService.h"
#endif

#include <QDebug>

namespace Dopamine::Core::Platform {

PlatformMediaService &PlatformMediaService::instance() {
    static PlatformMediaService s_instance;
    return s_instance;
}

PlatformMediaService::PlatformMediaService(QObject *parent)
    : QObject(parent) {}

PlatformMediaService::~PlatformMediaService() = default;

void PlatformMediaService::initialize(
    Features::Playback::App::PlaybackController *playbackCtrl,
    Features::Queue::App::QueueController *queueCtrl) {

#if defined(DOPAMINE_HAS_DBUS)
    if (!m_mprisService) {
        m_mprisService = new Media::Mpris::MprisPlayerService(playbackCtrl, queueCtrl, this);
        connect(m_mprisService, &Media::Mpris::MprisPlayerService::raiseRequested,
                this, &PlatformMediaService::raiseRequested);
        connect(m_mprisService, &Media::Mpris::MprisPlayerService::quitRequested,
                this, &PlatformMediaService::quitRequested);

        if (m_mprisService->registerService()) {
            qInfo() << "[PlatformMediaService] MPRIS 2 D-Bus service successfully registered.";
        } else {
            qWarning() << "[PlatformMediaService] Could not register MPRIS 2 D-Bus service.";
        }
    }
#else
    Q_UNUSED(playbackCtrl);
    Q_UNUSED(queueCtrl);
#endif
}

} // namespace Dopamine::Core::Platform
