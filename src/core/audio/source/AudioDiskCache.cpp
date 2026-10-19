/**
 * Location: src/core/audio/source/AudioDiskCache.cpp
 * Function: Implementation of LRU disk caching for streamed audio files with
 * deterministic token-stripped hashing, directory management, and pruning.
 * All Rights Reserved isgaar
 */
#include "AudioDiskCache.h"
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QDateTime>

namespace Dopamine::Core::Audio::Source {

AudioDiskCache::AudioDiskCache(qint64 maxSizeBytes)
    : m_maxSizeBytes(maxSizeBytes) {
    QDir().mkpath(cacheDirectory());
}

QString AudioDiskCache::cacheDirectory() const {
    QString base = QStandardPaths::writableLocation(
        QStandardPaths::CacheLocation);
    return base + "/audio";
}

QString AudioDiskCache::cacheKey(const QUrl &url) const {
    QByteArray cleanPath = url.path().toUtf8();
    return QString::fromLatin1(
        QCryptographicHash::hash(cleanPath, QCryptographicHash::Sha256)
            .toHex());
}

QString AudioDiskCache::filePathForKey(const QString &key) const {
    return cacheDirectory() + "/" + key + ".audio";
}

bool AudioDiskCache::has(const QString &key) const {
    return QFileInfo::exists(filePathForKey(key));
}

QString AudioDiskCache::getFilePath(const QUrl &url) const {
    QString key = cacheKey(url);
    if (has(key)) {
        return filePathForKey(key);
    }
    return QString();
}

void AudioDiskCache::recordAccess(const QString &key) {
    QString p = filePathForKey(key);
    QFile f(p);
    if (f.open(QIODevice::ReadWrite)) {
        f.setFileTime(QDateTime::currentDateTime(),
                      QFileDevice::FileAccessTime);
    }
}

void AudioDiskCache::prune() {
    QDir dir(cacheDirectory());
    QFileInfoList list = dir.entryInfoList(QDir::Files, QDir::Time);
    qint64 total = 0;
    for (const auto &fi : list) {
        total += fi.size();
    }
    while (total > m_maxSizeBytes && !list.isEmpty()) {
        QFileInfo oldest = list.takeLast();
        total -= oldest.size();
        QFile::remove(oldest.absoluteFilePath());
    }
}

} // namespace Dopamine::Core::Audio::Source
