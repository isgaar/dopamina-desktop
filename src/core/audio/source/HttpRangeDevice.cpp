/**
 * Location: src/core/audio/source/HttpRangeDevice.cpp
 * Function: Implementation of seekable HTTP audio device buffering server
 * tracks into disk cache with bearer auth headers and local file access.
 * All Rights Reserved isgaar
 */
#include "HttpRangeDevice.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QFileInfo>

namespace Dopamine::Core::Audio::Source {

HttpRangeDevice::HttpRangeDevice(const QUrl &url,
                                 const QByteArray &bearerToken,
                                 AudioDiskCache *cache,
                                 QNetworkAccessManager *nam,
                                 QObject *parent)
    : QIODevice(parent), m_url(url), m_token(bearerToken),
      m_cache(cache), m_nam(nam) {}

HttpRangeDevice::~HttpRangeDevice() { close(); }

void HttpRangeDevice::ensureCachedLocally() {
    if (m_url.isLocalFile()) {
        m_localFile.setFileName(m_url.toLocalFile());
        return;
    }
    if (!m_cache || !m_nam) return;
    QString key = m_cache->cacheKey(m_url);
    QString path = m_cache->filePathForKey(key);
    if (!m_cache->has(key)) {
        QNetworkRequest req(m_url);
        if (!m_token.isEmpty()) {
            req.setRawHeader("Authorization", "Bearer " + m_token);
        }
        QEventLoop loop;
        QNetworkReply *reply = m_nam->get(req);
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        if (reply->error() == QNetworkReply::NoError) {
            QFile out(path);
            if (out.open(QIODevice::WriteOnly)) {
                out.write(reply->readAll());
                out.close();
            }
        }
        reply->deleteLater();
    }
    m_cache->recordAccess(key);
    m_localFile.setFileName(path);
}

bool HttpRangeDevice::open(OpenMode mode) {
    ensureCachedLocally();
    if (!m_localFile.open(mode)) return false;
    return QIODevice::open(mode);
}

void HttpRangeDevice::close() {
    m_localFile.close();
    QIODevice::close();
}

qint64 HttpRangeDevice::size() const { return m_localFile.size(); }
qint64 HttpRangeDevice::pos() const { return m_localFile.pos(); }
bool HttpRangeDevice::seek(qint64 pos) {
    return m_localFile.seek(pos) && QIODevice::seek(pos);
}
bool HttpRangeDevice::atEnd() const { return m_localFile.atEnd(); }

qint64 HttpRangeDevice::readData(char *data, qint64 maxlen) {
    return m_localFile.read(data, maxlen);
}

qint64 HttpRangeDevice::writeData(const char *, qint64) { return -1; }

} // namespace Dopamine::Core::Audio::Source
