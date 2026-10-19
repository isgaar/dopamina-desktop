/**
 * Location: tests/test_playback_controller.cpp
 * Function: Unit test suite validating PlaybackController transitions,
 * audio state management, EqualizerController gains, and synced lyrics.
 * All Rights Reserved isgaar
 */
#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>
#include "core/ApiClient.h"
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
    void play(const QUrl &u) override {
        m_u = u; m_play = true; emit stateChanged(true);
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
        Dopamine::Core::ApiClient client;
        PlaybackController ctrl(&client, &audio);

        ctrl.tuneStation("chill");
        QCOMPARE(ctrl.stationId(), QString("chill"));
        QCOMPARE(ctrl.playbackOrigin(), QString("radio"));

        ctrl.play();
        QVERIFY(ctrl.isPlaying());
        QVERIFY(audio.isPlaying());

        ctrl.stop();
        QVERIFY(!ctrl.isPlaying());
        QVERIFY(!audio.isPlaying());

        ctrl.playLibraryTrack(99, "Song", "Artist", "Album", "", 200);
        QCOMPARE(ctrl.playbackOrigin(), QString("library"));
        QCOMPARE(ctrl.trackTitle(), QString("Song"));

        ctrl.tuneStation("jazz");
        QCOMPARE(ctrl.playbackOrigin(), QString("radio"));
        QCOMPARE(ctrl.stationId(), QString("jazz"));

        ctrl.setVolume(72);
        QCOMPARE(ctrl.volume(), 72);
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
