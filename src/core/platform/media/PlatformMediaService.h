/**
 * Location: src/core/platform/media/PlatformMediaService.h
 * Function: Master desktop OS platform media facade encapsulating Linux MPRIS2,
 * Windows SMTC, and platform-specific desktop media key integrations.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_PLATFORM_MEDIA_PLATFORMMEDIASERVICE_H
#define DOPAMINE_CORE_PLATFORM_MEDIA_PLATFORMMEDIASERVICE_H

#include <QObject>

namespace Dopamine::Features::Playback::App { class PlaybackController; }
namespace Dopamine::Features::Queue::App { class QueueController; }

#if defined(DOPAMINE_HAS_DBUS)
namespace Dopamine::Core::Platform::Media::Mpris { class MprisPlayerService; }
#endif

namespace Dopamine::Core::Platform {

class PlatformMediaService : public QObject {
    Q_OBJECT

public:
    static PlatformMediaService &instance();
    explicit PlatformMediaService(QObject *parent = nullptr);
    ~PlatformMediaService() override;

    void initialize(
        Features::Playback::App::PlaybackController *playbackCtrl,
        Features::Queue::App::QueueController *queueCtrl);

signals:
    void raiseRequested();
    void quitRequested();

private:
#if defined(DOPAMINE_HAS_DBUS)
    Media::Mpris::MprisPlayerService *m_mprisService = nullptr;
#endif
};

} // namespace Dopamine::Core::Platform

#endif // DOPAMINE_CORE_PLATFORM_MEDIA_PLATFORMMEDIASERVICE_H
