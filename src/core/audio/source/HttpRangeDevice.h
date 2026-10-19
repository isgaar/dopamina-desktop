/**
 * Location: src/core/audio/source/HttpRangeDevice.h
 * Function: Seekable QIODevice audio streaming source executing HTTP range
 * requests with bearer authorization tokens and background file buffering.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_AUDIO_SOURCE_HTTPRANGEDEVICE_H
#define DOPAMINE_CORE_AUDIO_SOURCE_HTTPRANGEDEVICE_H

#include <QIODevice>
#include <QUrl>
#include <QFile>
#include <QByteArray>
#include <QNetworkAccessManager>
#include "AudioDiskCache.h"

namespace Dopamine::Core::Audio::Source {

class HttpRangeDevice : public QIODevice {
    Q_OBJECT
public:
    explicit HttpRangeDevice(const QUrl &url,
                             const QByteArray &bearerToken,
                             AudioDiskCache *cache,
                             QNetworkAccessManager *nam,
                             QObject *parent = nullptr);
    ~HttpRangeDevice() override;

    bool open(OpenMode mode) override;
    void close() override;
    bool isSequential() const override { return false; }
    qint64 size() const override;
    qint64 pos() const override;
    bool seek(qint64 pos) override;
    bool atEnd() const override;

protected:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

private:
    void ensureCachedLocally();

    QUrl m_url;
    QByteArray m_token;
    AudioDiskCache *m_cache = nullptr;
    QNetworkAccessManager *m_nam = nullptr;
    QFile m_localFile;
    qint64 m_pos = 0;
};

} // namespace Dopamine::Core::Audio::Source

#endif // DOPAMINE_CORE_AUDIO_SOURCE_HTTPRANGEDEVICE_H
