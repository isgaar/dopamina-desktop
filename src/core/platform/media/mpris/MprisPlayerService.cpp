/**
 * Location: src/core/platform/media/mpris/MprisPlayerService.cpp
 * Function: Implementation of MPRIS 2 D-Bus service coordinating playback
 * metadata, OS media key transport events, and remote desktop actions.
 * All Rights Reserved isgaar
 */
#include "MprisPlayerService.h"
#include "MprisRootAdaptor.h"
#include "MprisPlayerAdaptor.h"
#include "features/playback/app/PlaybackController.h"
#include "features/queue/app/QueueController.h"
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QDebug>

namespace Dopamine::Core::Platform::Media::Mpris {

MprisPlayerService::MprisPlayerService(
    Features::Playback::App::PlaybackController *playbackCtrl,
    Features::Queue::App::QueueController *queueCtrl,
    QObject *parent)
    : QObject(parent),
      m_playbackCtrl(playbackCtrl),
      m_queueCtrl(queueCtrl),
      m_playbackStatus(QStringLiteral("Stopped")) {

    m_rootAdaptor = new MprisRootAdaptor(this);
    m_playerAdaptor = new MprisPlayerAdaptor(this);

    if (m_playbackCtrl) {
        connect(m_playbackCtrl, &Features::Playback::App::PlaybackController::trackChanged,
                this, &MprisPlayerService::onTrackChanged);
        connect(m_playbackCtrl, &Features::Playback::App::PlaybackController::playStateChanged,
                this, &MprisPlayerService::onPlayStateChanged);
        connect(m_playbackCtrl, &Features::Playback::App::PlaybackController::volumeChanged,
                this, &MprisPlayerService::onVolumeChanged);
        connect(m_playbackCtrl, &Features::Playback::App::PlaybackController::userSeeked,
                this, &MprisPlayerService::onUserSeeked);
    }

    if (m_queueCtrl) {
        connect(m_queueCtrl, &Features::Queue::App::QueueController::queueChanged,
                this, &MprisPlayerService::onQueueChanged);
        connect(m_queueCtrl, &Features::Queue::App::QueueController::indexChanged,
                this, &MprisPlayerService::onQueueChanged);
        connect(m_queueCtrl, &Features::Queue::App::QueueController::shuffleChanged,
                this, &MprisPlayerService::onShuffleChanged);
        connect(m_queueCtrl, &Features::Queue::App::QueueController::repeatChanged,
                this, &MprisPlayerService::onRepeatChanged);
    }

    updateMetadata();
    m_playbackStatus = playbackStatus();
}

MprisPlayerService::~MprisPlayerService() {
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (bus.isConnected()) {
        bus.unregisterObject(QStringLiteral("/org/mpris/MediaPlayer2"));
        if (!m_registeredServiceName.isEmpty()) {
            bus.unregisterService(m_registeredServiceName);
        }
    }
}

bool MprisPlayerService::registerService() {
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        qWarning() << "[MprisPlayerService] DBus session bus is not connected.";
        return false;
    }

    if (!bus.registerObject(QStringLiteral("/org/mpris/MediaPlayer2"), this)) {
        qWarning() << "[MprisPlayerService] Failed to register object at /org/mpris/MediaPlayer2:"
                   << bus.lastError().message();
        return false;
    }

    QString serviceName = QStringLiteral("org.mpris.MediaPlayer2.dopamine");
    if (!bus.registerService(serviceName)) {
        serviceName = QStringLiteral("org.mpris.MediaPlayer2.dopamine.instance%1")
                          .arg(QCoreApplication::applicationPid());
        if (!bus.registerService(serviceName)) {
            qWarning() << "[MprisPlayerService] Failed to register service" << serviceName << ":"
                       << bus.lastError().message();
            return false;
        }
    }

    m_registeredServiceName = serviceName;
    return true;
}

QString MprisPlayerService::playbackStatus() const {
    if (!m_playbackCtrl) return QStringLiteral("Stopped");
    if (m_playbackCtrl->isPlaying()) {
        return QStringLiteral("Playing");
    }
    if (m_isExplicitlyStopped) {
        return QStringLiteral("Stopped");
    }
    if (m_playbackCtrl->trackId() > 0 || !m_playbackCtrl->trackTitle().isEmpty()) {
        return QStringLiteral("Paused");
    }
    return QStringLiteral("Stopped");
}

QString MprisPlayerService::loopStatus() const {
    if (!m_queueCtrl) return QStringLiteral("None");
    return m_queueCtrl->isRepeat() ? QStringLiteral("Playlist") : QStringLiteral("None");
}

void MprisPlayerService::setLoopStatus(const QString &val) {
    if (!m_queueCtrl) return;
    const bool wantRepeat = (val.compare(QLatin1String("None"), Qt::CaseInsensitive) != 0);
    if (m_queueCtrl->isRepeat() != wantRepeat) {
        m_queueCtrl->toggleRepeat();
    }
}

double MprisPlayerService::rate() const {
    return 1.0;
}

void MprisPlayerService::setRate(double) {
    // Standard playback rate is 1.0
}

bool MprisPlayerService::shuffle() const {
    return m_queueCtrl ? m_queueCtrl->isShuffle() : false;
}

void MprisPlayerService::setShuffle(bool val) {
    if (m_queueCtrl && m_queueCtrl->isShuffle() != val) {
        m_queueCtrl->toggleShuffle();
    }
}

QVariantMap MprisPlayerService::metadata() const {
    return m_metadata;
}

double MprisPlayerService::volume() const {
    if (!m_playbackCtrl) return 1.0;
    return qBound(0.0, m_playbackCtrl->volume() / 100.0, 1.0);
}

void MprisPlayerService::setVolume(double val) {
    if (!m_playbackCtrl || val < 0.0) return;
    const int clamped = qBound(0, qRound(val * 100.0), 100);
    m_playbackCtrl->setVolume(clamped);
}

qlonglong MprisPlayerService::position() const {
    if (!m_playbackCtrl) return 0;
    return static_cast<qlonglong>(m_playbackCtrl->position()) * 1000000LL;
}

bool MprisPlayerService::canGoNext() const {
    return m_queueCtrl ? !m_queueCtrl->queueTracks().isEmpty() : false;
}

bool MprisPlayerService::canGoPrevious() const {
    if (!m_queueCtrl) return false;
    return m_queueCtrl->currentIndex() > 0 || (m_playbackCtrl && m_playbackCtrl->position() > 3);
}

bool MprisPlayerService::canPlay() const {
    return true;
}

bool MprisPlayerService::canPause() const {
    return true;
}

bool MprisPlayerService::canSeek() const {
    return true;
}

void MprisPlayerService::next() {
    if (m_queueCtrl) {
        m_queueCtrl->next();
    }
}

void MprisPlayerService::previous() {
    if (m_playbackCtrl && m_playbackCtrl->position() > 3) {
        m_playbackCtrl->seek(0);
    } else if (m_queueCtrl) {
        m_queueCtrl->previous();
    }
}

void MprisPlayerService::pause() {
    if (m_playbackCtrl) {
        m_playbackCtrl->pause();
    }
}

void MprisPlayerService::playPause() {
    m_isExplicitlyStopped = false;
    if (m_playbackCtrl) {
        m_playbackCtrl->togglePlay();
    }
}

void MprisPlayerService::stop() {
    m_isExplicitlyStopped = true;
    if (m_playbackCtrl) {
        m_playbackCtrl->stop();
    }
    onPlayStateChanged();
}

void MprisPlayerService::play() {
    m_isExplicitlyStopped = false;
    if (m_playbackCtrl) {
        m_playbackCtrl->play();
    }
}

void MprisPlayerService::seek(qlonglong offsetUs) {
    if (!m_playbackCtrl) return;
    const qlonglong curUs = static_cast<qlonglong>(m_playbackCtrl->position()) * 1000000LL;
    const qlonglong targetUs = curUs + offsetUs;
    if (targetUs < 0) {
        m_playbackCtrl->seek(0);
    } else {
        const qlonglong durUs = static_cast<qlonglong>(m_playbackCtrl->duration()) * 1000000LL;
        if (durUs > 0 && targetUs >= durUs) {
            if (m_queueCtrl) m_queueCtrl->next();
        } else {
            m_playbackCtrl->seek(static_cast<int>(targetUs / 1000000LL));
        }
    }
}

void MprisPlayerService::setPosition(const QDBusObjectPath &trackId, qlonglong positionUs) {
    if (!m_playbackCtrl) return;
    const QString currentPath = QStringLiteral("/org/mpris/MediaPlayer2/Track/%1")
                                    .arg(m_playbackCtrl->trackId());
    if (trackId.path().isEmpty() ||
        trackId.path() == currentPath ||
        trackId.path() == QLatin1String("/org/mpris/MediaPlayer2/Track/current")) {
        const int targetSec = static_cast<int>(qMax(0LL, positionUs) / 1000000LL);
        m_playbackCtrl->seek(targetSec);
    }
}

void MprisPlayerService::openUri(const QString &uri) {
    if (uri.isEmpty() || !m_playbackCtrl) return;
    const QFileInfo fi(uri);
    m_playbackCtrl->playLibraryTrack(0, fi.completeBaseName(), QString{}, QString{}, uri, 0);
}

void MprisPlayerService::raise() {
    emit raiseRequested();
}

void MprisPlayerService::quit() {
    emit quitRequested();
}

void MprisPlayerService::onTrackChanged() {
    m_isExplicitlyStopped = false;
    updateMetadata();
    const QString prevStatus = m_playbackStatus;
    m_playbackStatus = playbackStatus();

    QVariantMap changed;
    changed.insert(QStringLiteral("Metadata"), m_metadata);
    if (m_playbackStatus != prevStatus) {
        changed.insert(QStringLiteral("PlaybackStatus"), m_playbackStatus);
    }
    changed.insert(QStringLiteral("CanGoNext"), canGoNext());
    changed.insert(QStringLiteral("CanGoPrevious"), canGoPrevious());
    notifyPropertiesChanged(changed);
}

void MprisPlayerService::onPlayStateChanged() {
    const QString newStatus = playbackStatus();
    if (m_playbackStatus != newStatus) {
        m_playbackStatus = newStatus;
        QVariantMap changed;
        changed.insert(QStringLiteral("PlaybackStatus"), m_playbackStatus);
        notifyPropertiesChanged(changed);
    }
}

void MprisPlayerService::onVolumeChanged() {
    QVariantMap changed;
    changed.insert(QStringLiteral("Volume"), volume());
    notifyPropertiesChanged(changed);
}

void MprisPlayerService::onUserSeeked(int seconds) {
    const qlonglong posUs = static_cast<qlonglong>(qMax(0, seconds)) * 1000000LL;
    if (m_playerAdaptor) {
        m_playerAdaptor->emitSeeked(posUs);
    }
}

void MprisPlayerService::onQueueChanged() {
    QVariantMap changed;
    changed.insert(QStringLiteral("CanGoNext"), canGoNext());
    changed.insert(QStringLiteral("CanGoPrevious"), canGoPrevious());
    notifyPropertiesChanged(changed);
}

void MprisPlayerService::onShuffleChanged() {
    QVariantMap changed;
    changed.insert(QStringLiteral("Shuffle"), shuffle());
    notifyPropertiesChanged(changed);
}

void MprisPlayerService::onRepeatChanged() {
    QVariantMap changed;
    changed.insert(QStringLiteral("LoopStatus"), loopStatus());
    notifyPropertiesChanged(changed);
}

void MprisPlayerService::updateMetadata() {
    QVariantMap meta;
    if (!m_playbackCtrl) {
        m_metadata = meta;
        return;
    }

    const int id = m_playbackCtrl->trackId();
    const QString title = m_playbackCtrl->trackTitle();
    const QString artist = m_playbackCtrl->trackArtist();
    const QString album = m_playbackCtrl->trackAlbum();
    const QString path = m_playbackCtrl->trackPath();
    const int durationSec = m_playbackCtrl->duration();
    const QString cover = m_playbackCtrl->trackCover();

    if (id > 0) {
        meta.insert(QStringLiteral("mpris:trackid"),
                    QVariant::fromValue(QDBusObjectPath(QStringLiteral("/org/mpris/MediaPlayer2/Track/%1").arg(id))));
    } else if (!title.isEmpty()) {
        meta.insert(QStringLiteral("mpris:trackid"),
                    QVariant::fromValue(QDBusObjectPath(QStringLiteral("/org/mpris/MediaPlayer2/Track/current"))));
    } else {
        meta.insert(QStringLiteral("mpris:trackid"),
                    QVariant::fromValue(QDBusObjectPath(QStringLiteral("/org/mpris/MediaPlayer2/Track/none"))));
    }

    if (durationSec > 0) {
        meta.insert(QStringLiteral("mpris:length"), static_cast<qlonglong>(durationSec) * 1000000LL);
    } else {
        meta.insert(QStringLiteral("mpris:length"), static_cast<qlonglong>(0));
    }

    if (!title.isEmpty()) {
        meta.insert(QStringLiteral("xesam:title"), title);
    }
    if (!artist.isEmpty()) {
        meta.insert(QStringLiteral("xesam:artist"), QStringList{artist});
        meta.insert(QStringLiteral("xesam:albumArtist"), QStringList{artist});
    }
    if (!album.isEmpty()) {
        meta.insert(QStringLiteral("xesam:album"), album);
    }
    if (!path.isEmpty()) {
        meta.insert(QStringLiteral("xesam:url"), path);
    }

    const QString artUrl = resolveArtUrl(cover, id);
    if (!artUrl.isEmpty()) {
        meta.insert(QStringLiteral("mpris:artUrl"), artUrl);
    }

    m_metadata = meta;
}

QString MprisPlayerService::resolveArtUrl(const QString &coverUrl, int) const {
    if (coverUrl.isEmpty()) return QString{};
    const QString trimmed = coverUrl.trimmed();
    if (trimmed.startsWith(QLatin1String("file://")) ||
        trimmed.startsWith(QLatin1String("http://")) ||
        trimmed.startsWith(QLatin1String("https://"))) {
        return trimmed;
    }
    if (QFile::exists(trimmed)) {
        return QUrl::fromLocalFile(trimmed).toString();
    }
    return QString{};
}

void MprisPlayerService::notifyPropertiesChanged(const QVariantMap &changed) {
    if (changed.isEmpty()) return;
    QDBusMessage signal = QDBusMessage::createSignal(
        QStringLiteral("/org/mpris/MediaPlayer2"),
        QStringLiteral("org.freedesktop.DBus.Properties"),
        QStringLiteral("PropertiesChanged")
    );
    signal << QStringLiteral("org.mpris.MediaPlayer2.Player");
    signal << changed;
    signal << QStringList{};
    QDBusConnection::sessionBus().send(signal);
}

} // namespace Dopamine::Core::Platform::Media::Mpris
