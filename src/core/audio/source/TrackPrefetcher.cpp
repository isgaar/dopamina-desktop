/**
 * Location: src/core/audio/source/TrackPrefetcher.cpp
 * Function: Implementation of background audio prefetcher saving upcoming
 * stream segments into disk cache with asynchronous reply handling.
 * All Rights Reserved isgaar
 */
#include "TrackPrefetcher.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>

namespace Dopamine::Core::Audio::Source {

TrackPrefetcher::TrackPrefetcher(AudioDiskCache *cache,
                                 QNetworkAccessManager *nam,
                                 QObject *parent)
    : QObject(parent), m_cache(cache), m_nam(nam) {}

bool TrackPrefetcher::isPrefetched(const QUrl &url) const {
    if (url.isLocalFile()) return true;
    if (!m_cache) return false;
    return m_cache->has(m_cache->cacheKey(url));
}

void TrackPrefetcher::prefetch(const QUrl &url) {
    if (!url.isValid() || isPrefetched(url)) {
        emit prefetchFinished(url, true);
        return;
    }
    if (!m_nam || !m_cache) {
        emit prefetchFinished(url, false);
        return;
    }
    QNetworkRequest req(url);
    if (!m_token.isEmpty()) {
        req.setRawHeader("Authorization", "Bearer " + m_token);
    }
    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, url]() {
        bool ok = false;
        if (reply->error() == QNetworkReply::NoError) {
            QString path = m_cache->filePathForKey(m_cache->cacheKey(url));
            QFile out(path);
            if (out.open(QIODevice::WriteOnly)) {
                out.write(reply->readAll());
                out.close();
                ok = true;
            }
        }
        reply->deleteLater();
        emit prefetchFinished(url, ok);
    });
}

} // namespace Dopamine::Core::Audio::Source
