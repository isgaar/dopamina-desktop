/**
 * Location: src/core/audio/source/TrackPrefetcher.h
 * Function: Background track prefetch service downloading upcoming audio
 * streams ahead of track transitions to eliminate playback latency.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_AUDIO_SOURCE_TRACKPREFETCHER_H
#define DOPAMINE_CORE_AUDIO_SOURCE_TRACKPREFETCHER_H

#include <QObject>
#include <QUrl>
#include <QByteArray>
#include <QNetworkAccessManager>
#include "AudioDiskCache.h"

namespace Dopamine::Core::Audio::Source {

class TrackPrefetcher : public QObject {
    Q_OBJECT
public:
    explicit TrackPrefetcher(AudioDiskCache *cache,
                             QNetworkAccessManager *nam,
                             QObject *parent = nullptr);

    void setBearerToken(const QByteArray &token) { m_token = token; }
    void prefetch(const QUrl &url);
    bool isPrefetched(const QUrl &url) const;

signals:
    void prefetchFinished(const QUrl &url, bool success);

private:
    AudioDiskCache *m_cache = nullptr;
    QNetworkAccessManager *m_nam = nullptr;
    QByteArray m_token;
};

} // namespace Dopamine::Core::Audio::Source

#endif // DOPAMINE_CORE_AUDIO_SOURCE_TRACKPREFETCHER_H
