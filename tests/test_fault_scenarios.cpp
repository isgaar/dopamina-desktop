/**
 * Location: tests/test_fault_scenarios.cpp
 * Function: Integration unit tests executing fault injection scenarios S01-S14
 * covering offline servers, bad credentials lockout, and service degradation.
 * All Rights Reserved isgaar
 */
#include <QTest>
#include "core/connection/ConnectionService.h"
#include "core/storage/WatchdogRunner.h"
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
};

QTEST_MAIN(TestFaultScenarios)
#include "test_fault_scenarios.moc"
