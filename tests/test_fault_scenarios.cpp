/**
 * Location: tests/test_fault_scenarios.cpp
 * Function: Integration unit tests executing fault injection scenarios S01-S14
 * covering offline servers, bad credentials lockout, and service degradation.
 * All Rights Reserved isgaar
 */
#include <QTest>
#include <QJsonArray>
#include <QJsonObject>
#include "core/ApiClient.h"
#include "core/ProjectConfig.h"
#include "core/connection/ConnectionService.h"
#include "core/storage/WatchdogRunner.h"
#include "features/library/infra/LibraryApiClient.h"
#include "faults/FaultyStorageProvider.h"
#include "faults/FakeApiServer.h"

using namespace Core::Connection;
using namespace Core::Errors;

class TestFaultScenarios : public QObject {
    Q_OBJECT
private slots:
    void test_S01_server_offline_on_startup() {
        ConnectionService svc;
        ConnectionReport r;
        r.level = ServiceLevel::CacheOnly;
        r.code = ErrorCode::ConnTimeout;
        svc.updateReport(r);

        QCOMPARE(svc.state(), ConnectionState::Disconnected);
        QCOMPARE(svc.level(), static_cast<int>(ServiceLevel::CacheOnly));
        QVERIFY(!svc.isConnected());
    }

    void test_S03_bad_credentials_zero_automatic_retries() {
        ConnectionService svc;
        ConnectionReport r;
        r.level = ServiceLevel::CacheOnly;
        r.code = ErrorCode::AuthBadCredentials;
        svc.updateReport(r);

        QCOMPARE(svc.state(), ConnectionState::AuthFailed);
        QCOMPARE(svc.stateString(), QStringLiteral("AuthFallida"));
        QVERIFY(!svc.isConnected());
    }

    void test_S05_api_down_smb_healthy_degraded_level_b() {
        ConnectionService svc;
        ConnectionReport r;
        r.level = ServiceLevel::SmbOnly;
        r.transport = QStringLiteral("SMB");
        r.code = ErrorCode::NoError;
        svc.updateReport(r);

        QCOMPARE(svc.state(), ConnectionState::Degraded);
        QCOMPARE(svc.levelString(), QStringLiteral("Solo SMB"));
        QVERIFY(svc.isConnected());
    }

    void test_S06_smb_down_api_healthy_cache_mode() {
        ConnectionService svc;
        ConnectionReport r;
        r.level = ServiceLevel::CacheOnly;
        r.transport = QStringLiteral("HTTP");
        r.code = ErrorCode::ConnLost;
        svc.updateReport(r);

        QCOMPARE(svc.state(), ConnectionState::Disconnected);
        QCOMPARE(svc.level(), static_cast<int>(ServiceLevel::CacheOnly));
    }

    void test_S07_watchdog_abandons_hung_storage() {
        Core::Storage::WatchdogRunner watchdog;
        bool callbackFired = false;
        bool okResult = true;
        ErrorCode reportedError = ErrorCode::NoError;

        watchdog.executeWithDeadline(
            []() -> bool {
                QThread::msleep(500); // Exceeds deadline
                return true;
            },
            100, // 100 ms deadline
            [&](bool ok, ErrorCode err) {
                callbackFired = true;
                okResult = ok;
                reportedError = err;
            }
        );

        QTRY_VERIFY_WITH_TIMEOUT(callbackFired, 1000);
        QVERIFY(!okResult);
        QCOMPARE(reportedError, ErrorCode::ConnTimeout);
    }

    void test_S10_retry_after_trips_circuit_breaker() {
        CircuitBreaker cb;
        QVERIFY(cb.allowRequest());
        cb.setRetryAfterSeconds(2.0);
        QVERIFY(!cb.allowRequest()); // Must be blocked during Retry-After
    }

    void testApiClientResolvesRelativePaths() {
        Tests::Faults::FakeApiServer server;
        QVERIFY(server.start());
        Dopamine::Core::ProjectConfig::instance().setApiUrl(server.url());

        Dopamine::Core::ApiClient client;
        client.setBearerToken(QStringLiteral("test-token"));
        bool done = false;
        bool ok = false;
        QByteArray payload;
        client.get(QStringLiteral("/api/v1/health"),
                   [&](bool success, const QByteArray &data) {
                       done = true;
                       ok = success;
                       payload = data;
                   });

        QTRY_VERIFY_WITH_TIMEOUT(done, 2000);
        QVERIFY(ok);
        const QByteArray request = server.lastRequest();
        QVERIFY(request.startsWith("GET /api/v1/health HTTP/1.1"));
        QVERIFY(request.contains("Authorization: Bearer test-token"));
        QCOMPARE(payload, QByteArray(
            R"({"status":"ok","api_version":"1.0.0"})"));

        Dopamine::Core::ProjectConfig::instance().setApiUrl(
            QStringLiteral("http://127.0.0.1:8000"));
    }

    void testLibraryTracksUseAndroidDto() {
        Tests::Faults::FakeApiServer server;
        QVERIFY(server.start());
        Dopamine::Core::ProjectConfig::instance().setApiUrl(server.url());
        server.setNextResponse(200, QByteArrayLiteral(
            "[{\"id\":7,\"titulo\":\"One\","
            "\"streamUrl\":\"http://server/streams/7\"},"
            "{\"id\":8,\"titulo\":\"Two\","
            "\"stream_url\":\"http://server/streams/8\"}]"));

        Dopamine::Core::ApiClient client;
        Dopamine::Features::Library::Infra::LibraryApiClient library(&client);
        bool done = false;
        bool ok = false;
        QJsonArray tracks;
        library.fetchTracks(2, 50, QStringLiteral("ada"),
                            [&](bool success, const QJsonArray &data) {
                                done = true;
                                ok = success;
                                tracks = data;
                            });

        QTRY_VERIFY_WITH_TIMEOUT(done, 2000);
        QVERIFY(ok);
        QCOMPARE(tracks.size(), 2);
        QCOMPARE(tracks.at(0).toObject().value("path").toString(),
                 QStringLiteral("http://server/streams/7"));
        QCOMPARE(tracks.at(1).toObject().value("path").toString(),
                 QStringLiteral("http://server/streams/8"));
        QVERIFY(server.lastRequest().startsWith(
            "GET /api/v1/tracks?limit=50&offset=50&q=ada HTTP/1.1"));

        Dopamine::Core::ProjectConfig::instance().setApiUrl(
            QStringLiteral("http://127.0.0.1:8000"));
    }
};

QTEST_MAIN(TestFaultScenarios)
#include "test_fault_scenarios.moc"
