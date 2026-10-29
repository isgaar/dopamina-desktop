/**
 * Location: tests/test_platform_media.cpp
 * Function: Unit test suite verifying MPRIS 2 platform media metadata,
 * playback state transitions, volume normalization, and transport commands.
 * All Rights Reserved isgaar
 */
#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>
#include "core/audio/IAudioBackend.h"
#include "features/playback/app/PlaybackController.h"
#include "features/queue/app/QueueController.h"
#include "core/platform/media/PlatformMediaService.h"
#include "core/platform/media/mpris/MprisPlayerService.h"
#include "core/platform/media/mpris/MprisPlayerAdaptor.h"
#include "core/platform/media/mpris/MprisRootAdaptor.h"

using namespace Dopamine::Features::Playback::App;
using namespace Dopamine::Features::Queue::App;
using namespace Dopamine::Core::Platform;
using namespace Dopamine::Core::Platform::Media::Mpris;

class MockAudioForMedia : public Dopamine::Core::Audio::IAudioBackend {
    Q_OBJECT
public:
    using IAudioBackend::IAudioBackend;
    bool m_play = false;
    QUrl m_u;
    int m_v = 80;
    qint64 m_p = 0;
    qint64 m_dur = 240000;

    void play(const QUrl &u) override {
        m_u = u;
        m_play = true;
        emit stateChanged(true);
    }
    void playAt(const QUrl &u, qint64 p) override {
        m_p = p;
        play(u);
    }
    void pause() override {
        m_play = false;
        emit stateChanged(false);
    }
    void stop() override {
        m_play = false;
        emit stateChanged(false);
    }
    void setVolume(int v) override {
        m_v = v;
        emit volumeChanged(m_v);
    }
    int volume() const override { return m_v; }
    bool isPlaying() const override { return m_play; }
    QUrl currentUrl() const override { return m_u; }
    qint64 position() const override { return m_p; }
    void seek(qint64 p) override {
        m_p = p;
        emit positionChanged(m_p);
    }
    qint64 duration() const override { return m_dur; }
    void setNext(const QUrl &) override {}
    void setGainDb(float) override {}
    void setCrossfadeMs(int) override {}
};

class TestPlatformMedia : public QObject {
    Q_OBJECT

private slots:
    void testInitialStoppedState() {
        MockAudioForMedia audio;
        PlaybackController playbackCtrl(&audio);
        MprisPlayerService mpris(&playbackCtrl, nullptr);

        QCOMPARE(mpris.playbackStatus(), QStringLiteral("Stopped"));
        QCOMPARE(mpris.position(), 0);
        QCOMPARE(mpris.volume(), playbackCtrl.volume() / 100.0);
        QCOMPARE(mpris.loopStatus(), QStringLiteral("None"));
        QCOMPARE(mpris.shuffle(), false);

        QVariantMap meta = mpris.metadata();
        QVERIFY(meta.contains(QStringLiteral("mpris:trackid")));
    }

    void testMetadataExtractionAndFormatting() {
        MockAudioForMedia audio;
        PlaybackController playbackCtrl(&audio);
        MprisPlayerService mpris(&playbackCtrl, nullptr);

        playbackCtrl.playLibraryTrack(
            105,
            QStringLiteral("Get Lucky"),
            QStringLiteral("Daft Punk"),
            QStringLiteral("Random Access Memories"),
            QStringLiteral("/music/get_lucky.flac"),
            248,
            QStringLiteral("http://server.local/api/v1/covers/105")
        );

        QCOMPARE(mpris.playbackStatus(), QStringLiteral("Playing"));

        QVariantMap meta = mpris.metadata();
        QCOMPARE(meta.value(QStringLiteral("xesam:title")).toString(), QStringLiteral("Get Lucky"));
        QCOMPARE(meta.value(QStringLiteral("xesam:album")).toString(), QStringLiteral("Random Access Memories"));
        QCOMPARE(meta.value(QStringLiteral("xesam:artist")).toStringList(), QStringList{QStringLiteral("Daft Punk")});
        QCOMPARE(meta.value(QStringLiteral("mpris:length")).toLongLong(), 248LL * 1000000LL);
        QCOMPARE(meta.value(QStringLiteral("mpris:artUrl")).toString(), QStringLiteral("http://server.local/api/v1/covers/105"));

        QDBusObjectPath trackIdPath = meta.value(QStringLiteral("mpris:trackid")).value<QDBusObjectPath>();
        QCOMPARE(trackIdPath.path(), QStringLiteral("/org/mpris/MediaPlayer2/Track/105"));
    }

    void testPlaybackStatusTransitions() {
        MockAudioForMedia audio;
        PlaybackController playbackCtrl(&audio);
        MprisPlayerService mpris(&playbackCtrl, nullptr);

        playbackCtrl.playLibraryTrack(1, QStringLiteral("Track 1"), QStringLiteral("Artist"),
                                      QStringLiteral("Album"), QStringLiteral("/path.mp3"), 180);
        QCOMPARE(mpris.playbackStatus(), QStringLiteral("Playing"));

        mpris.pause();
        QCOMPARE(mpris.playbackStatus(), QStringLiteral("Paused"));

        mpris.play();
        QCOMPARE(mpris.playbackStatus(), QStringLiteral("Playing"));

        mpris.playPause();
        QCOMPARE(mpris.playbackStatus(), QStringLiteral("Paused"));

        mpris.stop();
        QCOMPARE(mpris.playbackStatus(), QStringLiteral("Stopped"));
    }

    void testVolumeScaling() {
        MockAudioForMedia audio;
        PlaybackController playbackCtrl(&audio);
        MprisPlayerService mpris(&playbackCtrl, nullptr);

        mpris.setVolume(0.50);
        QCOMPARE(playbackCtrl.volume(), 50);
        QCOMPARE(mpris.volume(), 0.50);

        mpris.setVolume(1.0);
        QCOMPARE(playbackCtrl.volume(), 100);
        QCOMPARE(mpris.volume(), 1.0);

        mpris.setVolume(0.0);
        QCOMPARE(playbackCtrl.volume(), 0);
        QCOMPARE(mpris.volume(), 0.0);
    }

    void testSeekCalculations() {
        MockAudioForMedia audio;
        PlaybackController playbackCtrl(&audio);
        MprisPlayerService mpris(&playbackCtrl, nullptr);

        playbackCtrl.playLibraryTrack(42, QStringLiteral("Song"), QStringLiteral("Artist"),
                                      QStringLiteral("Album"), QStringLiteral("/song.mp3"), 300);

        // Seek forward by 30 seconds (30,000,000 microseconds)
        mpris.seek(30LL * 1000000LL);
        QCOMPARE(playbackCtrl.position(), 30);

        // Set absolute position to 120 seconds
        mpris.setPosition(QDBusObjectPath(QStringLiteral("/org/mpris/MediaPlayer2/Track/42")),
                          120LL * 1000000LL);
        QCOMPARE(playbackCtrl.position(), 120);

        // Negative seek further back than start clamps to 0
        mpris.seek(-200LL * 1000000LL);
        QCOMPARE(playbackCtrl.position(), 0);
    }

    void testRaiseAndQuitSignals() {
        MockAudioForMedia audio;
        PlaybackController playbackCtrl(&audio);
        MprisPlayerService mpris(&playbackCtrl, nullptr);

        QSignalSpy spyRaise(&mpris, &MprisPlayerService::raiseRequested);
        QSignalSpy spyQuit(&mpris, &MprisPlayerService::quitRequested);

        mpris.raise();
        QCOMPARE(spyRaise.count(), 1);

        mpris.quit();
        QCOMPARE(spyQuit.count(), 1);
    }
};

QTEST_MAIN(TestPlatformMedia)
#include "test_platform_media.moc"
