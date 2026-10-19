/**
 * Location: src/core/audio/IAudioBackend.h
 * Function: Abstract audio playback interface decoupling client media decoding
 * from platform audio subsystems and providing unified stream state management.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_AUDIO_IAUDIOBACKEND_H
#define DOPAMINE_CORE_AUDIO_IAUDIOBACKEND_H

#include <QObject>
#include <QUrl>
#include <QString>

namespace Dopamine::Core::Audio {

class IAudioBackend : public QObject {
    Q_OBJECT
public:
    explicit IAudioBackend(QObject *parent = nullptr) : QObject(parent) {}
    ~IAudioBackend() override = default;

    virtual void play(const QUrl &url) = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual void seek(qint64 positionMs) = 0;
    virtual qint64 duration() const = 0;
    virtual void setVolume(int volume) = 0;
    virtual int volume() const = 0;
    virtual bool isPlaying() const = 0;
    virtual QUrl currentUrl() const = 0;
    virtual qint64 position() const = 0;
    virtual void setNext(const QUrl &url) = 0;
    virtual void setGainDb(float gainDb) = 0;
    virtual void setCrossfadeMs(int ms) = 0;

signals:
    void stateChanged(bool isPlaying);
    void errorOccurred(const QString &errorMessage);
    void volumeChanged(int volume);
    void positionChanged(qint64 positionMs);
    void durationChanged(qint64 durationMs);
    void trackFinished();
    void icyMetadataReceived(const QString &artist, const QString &title);
};

} // namespace Dopamine::Core::Audio

#endif // DOPAMINE_CORE_AUDIO_IAUDIOBACKEND_H
