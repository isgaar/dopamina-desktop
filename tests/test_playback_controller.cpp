/**
 * Location: tests/test_playback_controller.cpp
 * Function: Unit test suite validating library playback transitions, volume,
 * local lyric loading, and EqualizerController gains.
 * All Rights Reserved isgaar
 */
#include <QCoreApplication>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include "core/audio/IAudioBackend.h"
#include "features/playback/app/PlaybackController.h"
#include "features/playback/app/EqualizerController.h"
#include "features/playback/domain/LrcParser.h"

using namespace Dopamine::Features::Playback::App;
using namespace Dopamine::Features::Playback::Domain;

class MockAudio : public Dopamine::Core::Audio::IAudioBackend {
    Q_OBJECT
public:
    using IAudioBackend::IAudioBackend;
    bool m_play = false; QUrl m_u; int m_v = 80; qint64 m_p = 0;
    qint64 m_lastStart = -1;
    void play(const QUrl &u) override {
        m_u = u; m_play = true; emit stateChanged(true);
    }
    void playAt(const QUrl &u, qint64 p) override {
        m_lastStart = p;
        play(u);
    }
    void pause() override { m_play = false; emit stateChanged(false); }
    void stop() override { m_play = false; emit stateChanged(false); }
    void setVolume(int v) override { m_v = v; emit volumeChanged(m_v); }
    int volume() const override { return m_v; }
    bool isPlaying() const override { return m_play; }
    QUrl currentUrl() const override { return m_u; }
    qint64 position() const override { return m_p; }
    void seek(qint64 p) override { m_p = p; emit positionChanged(m_p); }
    qint64 duration() const override { return 180000; }
    void setNext(const QUrl &) override {}
    void setGainDb(float) override {}
    void setCrossfadeMs(int) override {}

    void tick(qint64 positionMs) {
        m_p = positionMs;
        emit positionChanged(positionMs);
    }
};

class TestPlaybackController : public QObject {
    Q_OBJECT
private slots:

    void testLrcParserAndActiveLine() {
        QString lrc = "[00:10.50]Line 1\n[00:25.00]Line 2\n[01:00.00]Line 3";
        auto lines = LrcParser::parse(lrc);
        QCOMPARE(lines.size(), 3);
        QCOMPARE(lines[0].text, QString("Line 1"));
        QCOMPARE(LrcParser::activeLineIndex(lines, 5000), -1);
        QCOMPARE(LrcParser::activeLineIndex(lines, 15000), 0);
        QCOMPARE(LrcParser::activeLineIndex(lines, 30000), 1);
        QCOMPARE(LrcParser::activeLineIndex(lines, 70000), 2);
    }

    void testPlaybackStateMachine() {
        MockAudio audio;
        PlaybackController ctrl(&audio);

        ctrl.playLibraryTrack(99, "Song", "Artist", "Album",
                              "/tmp/dopamine-song.mp3", 200);
        QCOMPARE(ctrl.trackTitle(), QString("Song"));
        QCOMPARE(ctrl.trackArtist(), QString("Artist"));
        QCOMPARE(audio.currentUrl(),
                 QUrl::fromLocalFile("/tmp/dopamine-song.mp3"));
        QVERIFY(ctrl.isPlaying());
        QVERIFY(audio.isPlaying());

        ctrl.pause();
        QVERIFY(!ctrl.isPlaying());
        QVERIFY(!audio.isPlaying());

        ctrl.togglePlay();
        QVERIFY(ctrl.isPlaying());

        ctrl.stop();
        QVERIFY(!ctrl.isPlaying());
        QVERIFY(!audio.isPlaying());

        ctrl.setVolume(72);
        QCOMPARE(ctrl.volume(), 72);

        ctrl.playLibraryTrack(100, "Other", "Artist", "Album", "", 0);
        QCOMPARE(ctrl.trackTitle(), QString("Other"));
    }

    void testRestoresRemoteQueuePosition() {
        MockAudio audio;
        PlaybackController ctrl(&audio);

        ctrl.loadLibraryTrack(7, "Remote", "Artist", "Album",
                              "http://server/api/v1/streams/7", 180, 42);
        QCOMPARE(ctrl.trackTitle(), QString("Remote"));
        QCOMPARE(ctrl.position(), 42);
        QVERIFY(!audio.isPlaying());
        QCOMPARE(audio.m_lastStart, qint64(-1));

        ctrl.play();
        QVERIFY(audio.isPlaying());
        QCOMPARE(audio.m_lastStart, qint64(42000));

        audio.tick(55000);
        ctrl.pause();
        ctrl.play();
        QCOMPARE(audio.m_lastStart, qint64(55000));
    }

    void testLocalLyrics() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QFile lrc(dir.filePath("song.lrc"));
        QVERIFY(lrc.open(QIODevice::WriteOnly | QIODevice::Text));
        lrc.write("[00:10.00]First line\n[00:20.00]Second line\n");
        lrc.close();

        MockAudio audio;
        PlaybackController ctrl(&audio);
        ctrl.playLibraryTrack(1, "Song", "Artist", "Album",
                              dir.filePath("song.mp3"), 180);

        QVERIFY(ctrl.lyricsAvailable());
        QCOMPARE(ctrl.lyricsLines().size(), 2);
        QCOMPARE(ctrl.activeLyricIndex(), -1);
        audio.tick(12000);
        QCOMPARE(ctrl.activeLyricIndex(), 0);
        audio.tick(22000);
        QCOMPARE(ctrl.activeLyricIndex(), 1);
    }

    void testEqualizerStateAndController() {
        EqualizerController eq;
        eq.reset();
        QCOMPARE(eq.bandLabels().size(), 12);
        for (int i = 0; i < 12; ++i) {
            QCOMPARE(eq.bandGains()[i].toDouble(), 0.0);
        }
        QCOMPARE(eq.preamp(), 0.0);
        QCOMPARE(eq.zBass(), 0.0);
        QCOMPARE(eq.compressor(), 0.0);
        eq.setBandGain(0, 3.5);
        QCOMPARE(eq.bandGains()[0].toDouble(), 3.5);
        eq.setPreamp(2.0);
        QCOMPARE(eq.preamp(), 2.0);
        eq.setZBass(45.0);
        QCOMPARE(eq.zBass(), 45.0);
        eq.setEffect1Knob(1, 28.0);
        QCOMPARE(eq.compressor(), 28.0);
        eq.reset();
        QCOMPARE(eq.zBass(), 0.0);
        QCOMPARE(eq.compressor(), 0.0);
        QCOMPARE(eq.bandGains()[0].toDouble(), 0.0);
    }
};

QTEST_MAIN(TestPlaybackController)
#include "test_playback_controller.moc"
