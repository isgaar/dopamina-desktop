/**
 * Location: tests/test_session_playback.cpp
 * Function: Unit tests for remote session selection, queue mirroring, saved
 * session fallback, and the paused restoration hand-off to the player.
 * All Rights Reserved isgaar
 */
#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>
#include "core/ProjectConfig.h"
#include "features/interconnect/app/SessionPlaybackController.h"
#include "features/interconnect/infra/SessionPlaybackApiClient.h"
#include "features/queue/app/QueueController.h"
#include "features/queue/infra/QueueApiClient.h"

using namespace Dopamine::Features::Interconnect::App;
using namespace Dopamine::Features::Interconnect::Infra;
using namespace Dopamine::Features::Queue::App;

namespace {

QJsonObject makeState(int trackId, const QList<int> &queue, double position,
                      bool playing, double updatedAt) {
    QJsonArray ids;
    for (int id : queue) ids.append(id);
    QJsonObject state;
    state.insert(QStringLiteral("track_id"), trackId);
    state.insert(QStringLiteral("active_queue"), ids);
    state.insert(QStringLiteral("position_sec"), position);
    state.insert(QStringLiteral("is_playing"), playing);
    state.insert(QStringLiteral("updated_at"), updatedAt);
    return state;
}

QJsonObject makeSaved(int trackId, const QList<int> &queue, double position,
                      double savedAt) {
    QJsonObject session;
    session.insert(QStringLiteral("track_id"), trackId);
    QJsonArray ids;
    for (int id : queue) ids.append(id);
    session.insert(QStringLiteral("active_queue"), ids);
    session.insert(QStringLiteral("position_sec"), position);
    session.insert(QStringLiteral("saved_at"), savedAt);
    return session;
}

class FakeSessionApi : public SessionPlaybackApiClient {
public:
    FakeSessionApi() : SessionPlaybackApiClient(nullptr) {}

    void fetchState(const QString &userId, StateCallback cb) override {
        cb(true, userId == QLatin1String("mobile_client") ? mobileState
                                                          : defaultState);
    }

    void fetchSavedSession(const QString &userId, StateCallback cb) override {
        cb(true, userId == QLatin1String("mobile_client") ? mobileSaved
                                                          : defaultSaved);
    }

    void resolveTracks(const QList<int> &ids, TracksCallback cb) override {
        resolvedIds = ids;
        QJsonArray tracks;
        for (int id : ids) {
            QJsonObject track;
            track.insert(QStringLiteral("id"), id);
            track.insert(QStringLiteral("title"),
                         QStringLiteral("Track %1").arg(id));
            track.insert(QStringLiteral("artist"), QStringLiteral("Artist"));
            track.insert(QStringLiteral("duration"), 120);
            track.insert(QStringLiteral("streamUrl"),
                         QStringLiteral("http://server/%1").arg(id));
            tracks.append(track);
        }
        cb(true, tracks);
    }

    void publishState(const QJsonObject &payload) override {
        publishedState = payload;
    }

    QJsonObject defaultState;
    QJsonObject mobileState;
    QJsonObject defaultSaved;
    QJsonObject mobileSaved;
    QJsonObject publishedState;
    QList<int> resolvedIds;
};

} // namespace

class TestSessionPlayback : public QObject {
    Q_OBJECT

private:
    static QVariantList queueTracks(QueueController &queue) {
        return queue.queueTracks();
    }

private slots:
    void testNewestMobileStateWinsAndRestoresInPause() {
        FakeSessionApi api;
        api.defaultState = makeState(
            6, {5, 6}, 30.0, true, 100.0);
        api.mobileState = makeState(
            9, {9}, 12.0, false, 200.0);

        Dopamine::Core::ApiClient client;
        Dopamine::Features::Queue::Infra::QueueApiClient queueApi(&client);
        QueueController queue(&queueApi);
        SessionPlaybackController controller(&api, &queue);

        QSignalSpy requested(&queue, &QueueController::sessionTrackRequested);
        controller.start();

        QCOMPARE(controller.hasSession(), true);
        QCOMPARE(controller.sourceKind(), QStringLiteral("mobile"));
        QCOMPARE(controller.remotePlaying(), false);
        QCOMPARE(controller.queueSize(), 1);
        QCOMPARE(api.resolvedIds, QList<int>{9});
        QCOMPARE(queue.currentIndex(), 0);
        QCOMPARE(queueTracks(queue).size(), 1);

        QCOMPARE(requested.count(), 1);
        const QList<QVariant> args = requested.takeFirst();
        QCOMPARE(args.at(0).toInt(), 9);
        QCOMPARE(args.at(1).toString(), QStringLiteral("Track 9"));
        QCOMPARE(args.at(4).toString(), QStringLiteral("http://server/9"));
        QCOMPARE(args.at(5).toInt(), 120);
        QCOMPARE(args.at(6).toInt(), 12);

        controller.stop();
        QCOMPARE(controller.hasSession(), false);
        QCOMPARE(controller.queueSize(), 0);
    }

    void testSavedSessionFallbackUsesNewestSession() {
        FakeSessionApi api;
        api.defaultSaved = makeSaved(
            2, {1, 2}, 5.0, 10.0);
        api.mobileSaved = makeSaved(
            3, {3}, 7.0, 20.0);

        Dopamine::Core::ApiClient client;
        Dopamine::Features::Queue::Infra::QueueApiClient queueApi(&client);
        QueueController queue(&queueApi);
        SessionPlaybackController controller(&api, &queue);

        QSignalSpy requested(&queue, &QueueController::sessionTrackRequested);
        controller.start();

        QCOMPARE(controller.hasSession(), true);
        QCOMPARE(controller.sourceKind(), QStringLiteral("mobile"));
        QCOMPARE(controller.remotePlaying(), false);
        QCOMPARE(controller.queueSize(), 1);
        QCOMPARE(requested.count(), 1);
        QCOMPARE(requested.first().at(0).toInt(), 3);
        QCOMPARE(requested.first().at(6).toInt(), 7);
    }

    void testSameQueueOnlyUpdatesRemotePlaying() {
        FakeSessionApi api;
        api.mobileState = makeState(
            9, {9, 10}, 12.0, true, 200.0);

        Dopamine::Core::ApiClient client;
        Dopamine::Features::Queue::Infra::QueueApiClient queueApi(&client);
        QueueController queue(&queueApi);
        SessionPlaybackController controller(&api, &queue);

        QSignalSpy requested(&queue, &QueueController::sessionTrackRequested);
        controller.start();
        QCOMPARE(requested.count(), 1);

        api.mobileState = makeState(
            9, {9, 10}, 40.0, false, 300.0);
        controller.refreshNow();

        QCOMPARE(requested.count(), 1);
        QCOMPARE(controller.remotePlaying(), false);
        QCOMPARE(controller.queueSize(), 2);
    }

    void testRealTimeRemoteControlBetweenAndroidAndDesktop() {
        FakeSessionApi api;
        Dopamine::Core::ApiClient client;
        Dopamine::Features::Queue::Infra::QueueApiClient queueApi(&client);
        QueueController queue(&queueApi);
        SessionPlaybackController controller(&api, &queue);

        controller.syncLocalPlayback(10, 15, true, 80);
        QCOMPARE(controller.localPlaying(), true);
        QCOMPARE(controller.activeDeviceKind(), QStringLiteral("desktop"));

        QSignalSpy volSpy(&controller,
            &SessionPlaybackController::remoteVolumeCommandReceived);
        QSignalSpy seekSpy(&controller,
            &SessionPlaybackController::remoteSeekCommandReceived);

        QJsonObject cmd = makeState(10, {10}, 55.0, true, 500.0);
        cmd.insert(QStringLiteral("volume"), 0.45);
        cmd.insert(QStringLiteral("device_id"),
                   QStringLiteral("remote_cmd_to_desktop"));
        api.defaultState = cmd;
        controller.refreshNow();

        QCOMPARE(volSpy.count(), 1);
        QCOMPARE(volSpy.first().at(0).toInt(), 45);
        QCOMPARE(seekSpy.count(), 1);
        QCOMPARE(seekSpy.first().at(0).toInt(), 55);
        QCOMPARE(api.publishedState.value(QStringLiteral("device_id")).toString(),
                 QStringLiteral("desktop_client"));

        controller.sendRemoteCommandToAndroid(10, 72, true, 65);
        QCOMPARE(api.publishedState.value(QStringLiteral("user_id")).toString(),
                 QStringLiteral("mobile_client"));
        QCOMPARE(api.publishedState.value(QStringLiteral("device_id")).toString(),
                 QStringLiteral("remote_cmd_to_mobile"));
        QCOMPARE(api.publishedState.value(QStringLiteral("position_sec")).toInt(), 72);
    }

    void testLiveServerMirrorsRemoteQueue() {
        const QString base = qEnvironmentVariable("DOPAMINE_LIVE_API");
        if (base.isEmpty())
            QSKIP("Set DOPAMINE_LIVE_API to run the live mirror test");

        auto &config = Dopamine::Core::ProjectConfig::instance();
        config.setApiUrl(base);

        Dopamine::Core::ApiClient client;
        SessionPlaybackApiClient api(&client);
        Dopamine::Features::Queue::Infra::QueueApiClient queueApi(&client);
        QueueController queue(&queueApi);
        SessionPlaybackController controller(&api, &queue);

        QSignalSpy requested(&queue, &QueueController::sessionTrackRequested);
        controller.start();

        QTRY_VERIFY_WITH_TIMEOUT(requested.count() > 0, 15000);
        QVERIFY(controller.hasSession());
        QVERIFY(controller.queueSize() > 0);
        QVERIFY(!queueTracks(queue).isEmpty());

        controller.stop();
        config.setApiUrl(QStringLiteral("http://127.0.0.1:8000"));
    }
};

QTEST_MAIN(TestSessionPlayback)
#include "test_session_playback.moc"
