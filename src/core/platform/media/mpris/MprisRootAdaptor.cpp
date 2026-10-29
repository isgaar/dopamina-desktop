/**
 * Location: src/core/platform/media/mpris/MprisRootAdaptor.cpp
 * Function: Implementation of org.mpris.MediaPlayer2 root adaptor forwarding
 * desktop actions and introspection metadata to host media service.
 * All Rights Reserved isgaar
 */
#include "MprisRootAdaptor.h"
#include "MprisPlayerService.h"

namespace Dopamine::Core::Platform::Media::Mpris {

MprisRootAdaptor::MprisRootAdaptor(MprisPlayerService *parent)
    : QDBusAbstractAdaptor(parent), m_service(parent) {
    setAutoRelaySignals(true);
}

bool MprisRootAdaptor::canQuit() const {
    return true;
}

bool MprisRootAdaptor::fullscreen() const {
    return false;
}

void MprisRootAdaptor::setFullscreen(bool) {
    // Desktop window currently manages its own window geometry
}

bool MprisRootAdaptor::canSetFullscreen() const {
    return false;
}

bool MprisRootAdaptor::canRaise() const {
    return true;
}

bool MprisRootAdaptor::hasTrackList() const {
    return false;
}

QString MprisRootAdaptor::identity() const {
    return QStringLiteral("Dopamine");
}

QString MprisRootAdaptor::desktopEntry() const {
    return QStringLiteral("dopamine");
}

QStringList MprisRootAdaptor::supportedUriSchemes() const {
    return {QStringLiteral("file"), QStringLiteral("http"), QStringLiteral("https")};
}

QStringList MprisRootAdaptor::supportedMimeTypes() const {
    return {
        QStringLiteral("audio/mpeg"),
        QStringLiteral("audio/flac"),
        QStringLiteral("audio/mp4"),
        QStringLiteral("audio/aac"),
        QStringLiteral("audio/ogg"),
        QStringLiteral("audio/wav"),
        QStringLiteral("audio/x-wav")
    };
}

void MprisRootAdaptor::Raise() {
    if (m_service) {
        m_service->raise();
    }
}

void MprisRootAdaptor::Quit() {
    if (m_service) {
        m_service->quit();
    }
}

} // namespace Dopamine::Core::Platform::Media::Mpris
