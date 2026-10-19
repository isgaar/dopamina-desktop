/**
 * Location: src/core/audio/source/AudioDiskCache.h
 * Function: Least-recently-used audio file disk cache storing streamed tracks
 * locally without authorization tokens according to user storage quotas.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_AUDIO_SOURCE_AUDIODISKCACHE_H
#define DOPAMINE_CORE_AUDIO_SOURCE_AUDIODISKCACHE_H

#include <QString>
#include <QUrl>
#include <cstdint>

namespace Dopamine::Core::Audio::Source {

class AudioDiskCache {
public:
    explicit AudioDiskCache(qint64 maxSizeBytes = 512 * 1024 * 1024);

    void setMaxSizeBytes(qint64 bytes) { m_maxSizeBytes = bytes; }
    qint64 maxSizeBytes() const { return m_maxSizeBytes; }

    QString cacheKey(const QUrl &url) const;
    QString filePathForKey(const QString &key) const;

    bool has(const QString &key) const;
    QString getFilePath(const QUrl &url) const;

    void recordAccess(const QString &key);
    void prune();

private:
    QString cacheDirectory() const;
    qint64 m_maxSizeBytes;
};

} // namespace Dopamine::Core::Audio::Source

#endif // DOPAMINE_CORE_AUDIO_SOURCE_AUDIODISKCACHE_H
