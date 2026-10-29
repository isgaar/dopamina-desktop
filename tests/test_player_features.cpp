/**
 * Location: tests/test_player_features.cpp
 * Function: Unit test suite exercising LibraryController, QueueController, and
 * PlaylistController with mocked network responses and state transitions.
 * All Rights Reserved isgaar
 */
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>
#include <QSettings>
#include <QTemporaryFile>
#include "core/ApiClient.h"
#include "core/ProjectConfig.h"
#include "faults/FakeApiServer.h"
#include "features/library/app/LibraryController.h"
#include "features/library/infra/RecursiveDirectoryScanner.h"
#include "features/queue/app/QueueController.h"
#include "features/playlists/app/PlaylistController.h"

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

    void testTrackPaginationAppendsPages() {
        Tests::Faults::FakeApiServer server;
        QVERIFY(server.start());
        auto &cfg = Dopamine::Core::ProjectConfig::instance();
        cfg.setServerSource();
        cfg.setApiUrl(server.url());

        Dopamine::Core::ApiClient client;
        Dopamine::Features::Library::Infra::LibraryApiClient api(&client);
        LibraryController controller(&api);

        auto makePage = [](int firstId) {
            QJsonArray page;
            for (int i = 0; i < 100; ++i) {
                page.append(QJsonObject{
                    {"id", firstId + i},
                    {"title", QStringLiteral("Track %1").arg(firstId + i)}});
            }
            return QJsonDocument(page).toJson(QJsonDocument::Compact);
        };

        server.setNextResponse(200, makePage(1));
        QSignalSpy spy(&controller, &LibraryController::tracksChanged);
        controller.loadTracks(1);
        QVERIFY(spy.wait(3000));
        QCOMPARE(controller.tracks().size(), 100);
        QVERIFY(controller.hasMoreTracks());

        server.setNextResponse(200, makePage(101));
        controller.loadMoreTracks();
        QVERIFY(spy.wait(3000));
        QCOMPARE(controller.tracks().size(), 200);
        QVERIFY(controller.hasMoreTracks());

        QJsonArray lastPage;
        lastPage.append(QJsonObject{{"id", 201}, {"title", "Last"}});
        server.setNextResponse(
            200, QJsonDocument(lastPage).toJson(QJsonDocument::Compact));
        controller.loadMoreTracks();
        QVERIFY(spy.wait(3000));
        QCOMPARE(controller.tracks().size(), 201);
        QVERIFY(!controller.hasMoreTracks());

        cfg.setApiUrl(QStringLiteral("http://127.0.0.1:8000"));
    }

    void testMixDeduplicationAndAffinity() {
        Tests::Faults::FakeApiServer server;
        QVERIFY(server.start());
        auto &cfg = Dopamine::Core::ProjectConfig::instance();
        cfg.setApiUrl(server.url());

        // Pre-set affinity for seed 500:
        // Track 503 has high positive affinity (+5.0), Track 501 has neutral (0.0)
        {
            QSettings aff(QStringLiteral("DopamineFM"), QStringLiteral("MixAffinity"));
            aff.setValue(QStringLiteral("pair_500_503"), 5.0);
            aff.setValue(QStringLiteral("pair_500_501"), 0.0);
        }

        QJsonObject resp;
        QJsonArray tracks;
        // Track 1
        tracks.append(QJsonObject{
            {QStringLiteral("id"), 501},
            {QStringLiteral("title"), QStringLiteral("Rock That Body")},
            {QStringLiteral("artist"), QStringLiteral("The Black Eyed Peas")},
            {QStringLiteral("duration"), 240}
        });
        // Track 2: duplicate signature of 501 (Remastered version without 'The')
        tracks.append(QJsonObject{
            {QStringLiteral("id"), 502},
            {QStringLiteral("title"), QStringLiteral("Rock That Body (Remastered 2020)")},
            {QStringLiteral("artist"), QStringLiteral("Black Eyed Peas")},
            {QStringLiteral("duration"), 242}
        });
        // Track 3: distinct song with high affinity
        tracks.append(QJsonObject{
            {QStringLiteral("id"), 503},
            {QStringLiteral("title"), QStringLiteral("I Gotta Feeling")},
            {QStringLiteral("artist"), QStringLiteral("Black Eyed Peas")},
            {QStringLiteral("duration"), 280}
        });
        resp[QStringLiteral("tracks")] = tracks;
        resp[QStringLiteral("ids")] = QJsonArray{501, 502, 503};

        server.setNextResponse(200, QJsonDocument(resp).toJson(QJsonDocument::Compact));

        Dopamine::Core::ApiClient client;
        Dopamine::Features::Queue::Infra::QueueApiClient qApi(&client);
        QueueController queue(&qApi);

        QSignalSpy spy(&queue, &QueueController::queueChanged);
        queue.startMix(500, QStringLiteral("Boom Boom Pow"));

        // Wait for refillMixIfNeeded to complete via fake server
        QVERIFY(spy.wait(3000));

        // Queue should contain:
        // index 0: seed track 500 (Boom Boom Pow)
        // index 1: track 503 (I Gotta Feeling) - sorted first due to +5.0 affinity
        // index 2: track 501 (Rock That Body)
        // track 502 MUST be skipped because it is a duplicate signature of 501
        const auto qTracks = queue.queueTracks();
        QCOMPARE(qTracks.size(), 3);
        QCOMPARE(qTracks[0].toMap().value(QStringLiteral("id")).toInt(), 500);
        QCOMPARE(qTracks[1].toMap().value(QStringLiteral("id")).toInt(), 503);
        QCOMPARE(qTracks[2].toMap().value(QStringLiteral("id")).toInt(), 501);

        // Clean up QSettings
        {
            QSettings aff(QStringLiteral("DopamineFM"), QStringLiteral("MixAffinity"));
            aff.remove(QStringLiteral("pair_500_503"));
            aff.remove(QStringLiteral("pair_500_501"));
        }

        cfg.setApiUrl(QStringLiteral("http://127.0.0.1:8000"));
    }

    void testDownloadIntegrityAndDeduplication() {
        Dopamine::Core::ApiClient client;
        Dopamine::Features::Library::Infra::LibraryApiClient api(&client);
        LibraryController controller(&api);

        QTemporaryFile tempFile;
        QVERIFY(tempFile.open());
        QString tempPath = tempFile.fileName();
        tempFile.close();

        // Mark track 999 as downloaded in QSettings
        {
            QSettings dl(QStringLiteral("DopamineFM"), QStringLiteral("Downloads"));
            dl.setValue(QStringLiteral("track_999"), tempPath);
        }

        QVERIFY(controller.isTrackDownloaded(999));

        QSignalSpy spyDl(&controller, &LibraryController::trackDownloaded);
        controller.downloadTrack(999, QStringLiteral("Song"), QStringLiteral("flac"), QStringLiteral("http://fake/stream"));

        // Must immediately notify already_downloaded and not perform duplicate network work
        QCOMPARE(spyDl.count(), 1);
        QCOMPARE(spyDl.takeFirst().at(0).toString(), QStringLiteral("already_downloaded"));

        // Clean up QSettings
        {
            QSettings dl(QStringLiteral("DopamineFM"), QStringLiteral("Downloads"));
            dl.remove(QStringLiteral("track_999"));
        }
    }

    void testServerModeDoesNotScanSmbRoot() {
        auto &cfg = Dopamine::Core::ProjectConfig::instance();
        cfg.setSmbSource(QString(), QString(), QString(), QString());
        QCOMPARE(Dopamine::Features::Library::Infra::
                     RecursiveDirectoryScanner::resolveRootPath(),
                 QString());

        cfg.setServerSource();
        QCOMPARE(cfg.sourceType(), QStringLiteral("server"));
        QVERIFY(cfg.smbHost().isEmpty());
        QCOMPARE(Dopamine::Features::Library::Infra::
                     RecursiveDirectoryScanner::resolveRootPath(),
                 QString());
    }
};

QTEST_MAIN(TestPlayerFeatures)
#include "test_player_features.moc"
