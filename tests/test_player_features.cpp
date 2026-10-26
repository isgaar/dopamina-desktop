/**
 * Location: tests/test_player_features.cpp
 * Function: Unit test suite exercising LibraryController, QueueController, and
 * PlaylistController with mocked network responses and state transitions.
 * All Rights Reserved isgaar
 */
#include <QTest>
#include <QSignalSpy>
#include "features/library/app/LibraryController.h"
#include "features/queue/app/QueueController.h"
#include "features/playlists/app/PlaylistController.h"
#include "core/ApiClient.h"

using namespace Dopamine::Features::Library::App;
using namespace Dopamine::Features::Queue::App;
using namespace Dopamine::Features::Playlists::App;

class TestPlayerFeatures : public QObject {
    Q_OBJECT
private slots:
    void testQueueOperations() {
        Dopamine::Core::ApiClient client;
        Dopamine::Features::Queue::Infra::QueueApiClient qApi(&client);
        QueueController queue(&qApi);

        QVariantMap t1; t1["id"] = 101; t1["title"] = "Track A";
        QVariantMap t2; t2["id"] = 102; t2["title"] = "Track B";
        queue.appendToQueue(t1);
        queue.appendToQueue(t2);
        QCOMPARE(queue.queueTracks().size(), 2);

        queue.next();
        QCOMPARE(queue.currentIndex(), 0);
        queue.next();
        QCOMPARE(queue.currentIndex(), 1);

        queue.toggleShuffle();
        QVERIFY(queue.isShuffle());
        queue.toggleRepeat();
        QVERIFY(queue.isRepeat());

        queue.clearQueue();
        QCOMPARE(queue.queueTracks().size(), 0);
    }

    void testMixSeedQueueing() {
        Dopamine::Core::ApiClient client;
        Dopamine::Features::Queue::Infra::QueueApiClient qApi(&client);
        QueueController queue(&qApi);

        queue.startMix(42, "Seed Song");
        QVERIFY(queue.isMixActive());
        QCOMPARE(queue.mixSeedTitle(), QString("Seed Song"));
        QCOMPARE(queue.currentIndex(), 0);
        QVERIFY(queue.queueTracks().size() >= 1);
    }

    void testApplySessionQueueRestoresTrack() {
        Dopamine::Core::ApiClient client;
        Dopamine::Features::Queue::Infra::QueueApiClient qApi(&client);
        QueueController queue(&qApi);

        QVariantMap t1;
        t1["id"] = 201; t1["title"] = "Remote A"; t1["duration"] = 100;
        t1["path"] = "http://server/api/v1/streams/201";
        QVariantMap t2;
        t2["id"] = 202; t2["title"] = "Remote B"; t2["duration"] = 120;
        t2["path"] = "http://server/api/v1/streams/202";
        QVariantList tracks;
        tracks << t1 << t2;

        QSignalSpy spy(&queue,
                       &QueueController::sessionTrackRequested);
        queue.applySessionQueue(tracks, 1, 37);

        QCOMPARE(queue.queueTracks().size(), 2);
        QCOMPARE(queue.currentIndex(), 1);
        QCOMPARE(spy.count(), 1);
        const QList<QVariant> args = spy.takeFirst();
        QCOMPARE(args.at(0).toInt(), 202);
        QCOMPARE(args.at(1).toString(), QString("Remote B"));
        QCOMPARE(args.at(5).toInt(), 120);
        QCOMPARE(args.at(6).toInt(), 37);
    }

    void testPlaylistLikeToggle() {
        Dopamine::Core::ApiClient client;
        Dopamine::Features::Playlists::Infra::PlaylistApiClient plApi(&client);
        PlaylistController pl(&plApi);

        QVERIFY(!pl.isLiked(99));
        pl.toggleLike(99);
        QVERIFY(pl.isLiked(99));
        pl.toggleLike(99);
        QVERIFY(!pl.isLiked(99));
    }
};

QTEST_MAIN(TestPlayerFeatures)
#include "test_player_features.moc"
