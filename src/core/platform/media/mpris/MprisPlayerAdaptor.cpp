/**
 * Location: src/core/platform/media/mpris/MprisPlayerAdaptor.cpp
 * Function: Implementation of org.mpris.MediaPlayer2.Player D-Bus adaptor
 * translating D-Bus calls and properties to the internal playback coordinator.
 * All Rights Reserved isgaar
 */
#include "MprisPlayerAdaptor.h"
#include "MprisPlayerService.h"

namespace Dopamine::Core::Platform::Media::Mpris {

MprisPlayerAdaptor::MprisPlayerAdaptor(MprisPlayerService *parent)
    : QDBusAbstractAdaptor(parent), m_service(parent) {
    setAutoRelaySignals(true);
}

QString MprisPlayerAdaptor::playbackStatus() const {
    return m_service ? m_service->playbackStatus() : QStringLiteral("Stopped");
}

QString MprisPlayerAdaptor::loopStatus() const {
    return m_service ? m_service->loopStatus() : QStringLiteral("None");
}

void MprisPlayerAdaptor::setLoopStatus(const QString &val) {
    if (m_service) m_service->setLoopStatus(val);
}

double MprisPlayerAdaptor::rate() const {
    return m_service ? m_service->rate() : 1.0;
}

void MprisPlayerAdaptor::setRate(double val) {
    if (m_service) m_service->setRate(val);
}

bool MprisPlayerAdaptor::shuffle() const {
    return m_service ? m_service->shuffle() : false;
}

void MprisPlayerAdaptor::setShuffle(bool val) {
    if (m_service) m_service->setShuffle(val);
}

QVariantMap MprisPlayerAdaptor::metadata() const {
    return m_service ? m_service->metadata() : QVariantMap{};
}

double MprisPlayerAdaptor::volume() const {
    return m_service ? m_service->volume() : 1.0;
}

void MprisPlayerAdaptor::setVolume(double val) {
    if (m_service) m_service->setVolume(val);
}

qlonglong MprisPlayerAdaptor::position() const {
    return m_service ? m_service->position() : 0;
}

double MprisPlayerAdaptor::minimumRate() const {
    return 1.0;
}

double MprisPlayerAdaptor::maximumRate() const {
    return 1.0;
}

bool MprisPlayerAdaptor::canGoNext() const {
    return m_service ? m_service->canGoNext() : false;
}

bool MprisPlayerAdaptor::canGoPrevious() const {
    return m_service ? m_service->canGoPrevious() : false;
}

bool MprisPlayerAdaptor::canPlay() const {
    return m_service ? m_service->canPlay() : true;
}

bool MprisPlayerAdaptor::canPause() const {
    return m_service ? m_service->canPause() : true;
}

bool MprisPlayerAdaptor::canSeek() const {
    return m_service ? m_service->canSeek() : true;
}

bool MprisPlayerAdaptor::canControl() const {
    return true;
}

void MprisPlayerAdaptor::emitSeeked(qlonglong positionUs) {
    emit Seeked(positionUs);
}

void MprisPlayerAdaptor::Next() {
    if (m_service) m_service->next();
}

void MprisPlayerAdaptor::Previous() {
    if (m_service) m_service->previous();
}

void MprisPlayerAdaptor::Pause() {
    if (m_service) m_service->pause();
}

void MprisPlayerAdaptor::PlayPause() {
    if (m_service) m_service->playPause();
}

void MprisPlayerAdaptor::Stop() {
    if (m_service) m_service->stop();
}

void MprisPlayerAdaptor::Play() {
    if (m_service) m_service->play();
}

void MprisPlayerAdaptor::Seek(qlonglong Offset) {
    if (m_service) m_service->seek(Offset);
}

void MprisPlayerAdaptor::SetPosition(const QDBusObjectPath &TrackId, qlonglong Position) {
    if (m_service) m_service->setPosition(TrackId, Position);
}

void MprisPlayerAdaptor::OpenUri(const QString &Uri) {
    if (m_service) m_service->openUri(Uri);
}

} // namespace Dopamine::Core::Platform::Media::Mpris
