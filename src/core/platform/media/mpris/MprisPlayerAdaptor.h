/**
 * Location: src/core/platform/media/mpris/MprisPlayerAdaptor.h
 * Function: D-Bus adaptor exporting org.mpris.MediaPlayer2.Player interface for
 * remote media controls, track metadata, and position/seek synchronization.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_PLATFORM_MEDIA_MPRIS_MPRISPLAYERADAPTOR_H
#define DOPAMINE_CORE_PLATFORM_MEDIA_MPRIS_MPRISPLAYERADAPTOR_H

#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>
#include <QVariantMap>

namespace Dopamine::Core::Platform::Media::Mpris {

class MprisPlayerService;

class MprisPlayerAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")
    Q_CLASSINFO("D-Bus Introspection",
        "  <interface name=\"org.mpris.MediaPlayer2.Player\">\n"
        "    <method name=\"Next\"/>\n"
        "    <method name=\"Previous\"/>\n"
        "    <method name=\"Pause\"/>\n"
        "    <method name=\"PlayPause\"/>\n"
        "    <method name=\"Stop\"/>\n"
        "    <method name=\"Play\"/>\n"
        "    <method name=\"Seek\">\n"
        "      <arg direction=\"in\" type=\"x\" name=\"Offset\"/>\n"
        "    </method>\n"
        "    <method name=\"SetPosition\">\n"
        "      <arg direction=\"in\" type=\"o\" name=\"TrackId\"/>\n"
        "      <arg direction=\"in\" type=\"x\" name=\"Position\"/>\n"
        "    </method>\n"
        "    <method name=\"OpenUri\">\n"
        "      <arg direction=\"in\" type=\"s\" name=\"Uri\"/>\n"
        "    </method>\n"
        "    <signal name=\"Seeked\">\n"
        "      <arg type=\"x\" name=\"Position\"/>\n"
        "    </signal>\n"
        "    <property name=\"PlaybackStatus\" type=\"s\" access=\"read\"/>\n"
        "    <property name=\"LoopStatus\" type=\"s\" access=\"readwrite\"/>\n"
        "    <property name=\"Rate\" type=\"d\" access=\"readwrite\"/>\n"
        "    <property name=\"Shuffle\" type=\"b\" access=\"readwrite\"/>\n"
        "    <property name=\"Metadata\" type=\"a{sv}\" access=\"read\"/>\n"
        "    <property name=\"Volume\" type=\"d\" access=\"readwrite\"/>\n"
        "    <property name=\"Position\" type=\"x\" access=\"read\"/>\n"
        "    <property name=\"MinimumRate\" type=\"d\" access=\"read\"/>\n"
        "    <property name=\"MaximumRate\" type=\"d\" access=\"read\"/>\n"
        "    <property name=\"CanGoNext\" type=\"b\" access=\"read\"/>\n"
        "    <property name=\"CanGoPrevious\" type=\"b\" access=\"read\"/>\n"
        "    <property name=\"CanPlay\" type=\"b\" access=\"read\"/>\n"
        "    <property name=\"CanPause\" type=\"b\" access=\"read\"/>\n"
        "    <property name=\"CanSeek\" type=\"b\" access=\"read\"/>\n"
        "    <property name=\"CanControl\" type=\"b\" access=\"read\"/>\n"
        "  </interface>\n"
    )

    Q_PROPERTY(QString PlaybackStatus READ playbackStatus)
    Q_PROPERTY(QString LoopStatus READ loopStatus WRITE setLoopStatus)
    Q_PROPERTY(double Rate READ rate WRITE setRate)
    Q_PROPERTY(bool Shuffle READ shuffle WRITE setShuffle)
    Q_PROPERTY(QVariantMap Metadata READ metadata)
    Q_PROPERTY(double Volume READ volume WRITE setVolume)
    Q_PROPERTY(qlonglong Position READ position)
    Q_PROPERTY(double MinimumRate READ minimumRate)
    Q_PROPERTY(double MaximumRate READ maximumRate)
    Q_PROPERTY(bool CanGoNext READ canGoNext)
    Q_PROPERTY(bool CanGoPrevious READ canGoPrevious)
    Q_PROPERTY(bool CanPlay READ canPlay)
    Q_PROPERTY(bool CanPause READ canPause)
    Q_PROPERTY(bool CanSeek READ canSeek)
    Q_PROPERTY(bool CanControl READ canControl)

public:
    explicit MprisPlayerAdaptor(MprisPlayerService *parent);

    QString playbackStatus() const;
    QString loopStatus() const;
    void setLoopStatus(const QString &val);
    double rate() const;
    void setRate(double val);
    bool shuffle() const;
    void setShuffle(bool val);
    QVariantMap metadata() const;
    double volume() const;
    void setVolume(double val);
    qlonglong position() const;
    double minimumRate() const;
    double maximumRate() const;
    bool canGoNext() const;
    bool canGoPrevious() const;
    bool canPlay() const;
    bool canPause() const;
    bool canSeek() const;
    bool canControl() const;

    void emitSeeked(qlonglong positionUs);

public slots:
    void Next();
    void Previous();
    void Pause();
    void PlayPause();
    void Stop();
    void Play();
    void Seek(qlonglong Offset);
    void SetPosition(const QDBusObjectPath &TrackId, qlonglong Position);
    void OpenUri(const QString &Uri);

signals:
    void Seeked(qlonglong Position);

private:
    MprisPlayerService *m_service = nullptr;
};

} // namespace Dopamine::Core::Platform::Media::Mpris

#endif // DOPAMINE_CORE_PLATFORM_MEDIA_MPRIS_MPRISPLAYERADAPTOR_H
