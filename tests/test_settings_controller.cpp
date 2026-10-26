/**
 * Location: tests/test_settings_controller.cpp
 * Function: Unit test suite for SettingsController verifying draft mutations,
 * source modes, credential isolation in ICredentialStore, and first run flags.
 * All Rights Reserved isgaar
 */
#include <QCoreApplication>
#include <QFile>
#include <QSettings>
#include <QSignalSpy>
#include <QTest>
#include "core/ProjectConfig.h"
#include "core/credentials/MemoryCredentialStore.h"
#include "core/service/LocalServiceManager.h"
#include "features/settings/app/SettingsController.h"
#include "features/settings/infra/SettingsStorage.h"

using namespace Dopamine::Features::Settings::App;
using namespace Dopamine::Features::Settings::Infra;
using namespace Dopamine::Core;

class TestSettingsController : public QObject {
    Q_OBJECT

private slots:
    void init() {
        SettingsStorage::setTestScope(true);
        QSettings s(QStringLiteral("isgaar_test"), QStringLiteral("Dopamine"));
        s.clear();
        s.setValue(QStringLiteral("FirstRun/Completed"), true);
        QFile::remove(SettingsStorage::appearanceFilePath());
        ProjectConfig::instance().setApiUrl(
            QStringLiteral("http://127.0.0.1:8000"));
    }

    void cleanup() {
        QSettings s(QStringLiteral("isgaar_test"), QStringLiteral("Dopamine"));
        s.clear();
        QFile::remove(SettingsStorage::appearanceFilePath());
        SettingsStorage::setTestScope(false);
    }

    void testInitiallyClean() {
        SettingsController controller;
        QVERIFY(!controller.isDirty());
        QCOMPARE(controller.draftApiUrl(),
                 QStringLiteral("http://127.0.0.1:8000"));
    }

    void testDirtyActivatesOnEdit() {
        SettingsController controller;
        QSignalSpy spy(&controller, &SettingsController::dirtyChanged);
        controller.setDraftApiUrl(QStringLiteral("http://remote:9000"));
        QVERIFY(controller.isDirty());
        QCOMPARE(spy.count(), 1);
    }

    void testDiscardRestoresOriginal() {
        SettingsController controller;
        QString orig = controller.draftApiUrl();
        controller.setDraftApiUrl(QStringLiteral("http://remote:9000"));
        QVERIFY(controller.isDirty());

        controller.discard();
        QCOMPARE(controller.draftApiUrl(), orig);
        QVERIFY(!controller.isDirty());
    }

    void testApplyPersistsState() {
        SettingsController controller;
        controller.setDraftSourceType(QStringLiteral("server"));
        controller.setDraftApiUrl(QStringLiteral("http://persisted:7000"));
        QVERIFY(controller.isDirty());

        controller.apply();
        QVERIFY(!controller.isDirty());
        QCOMPARE(controller.draftApiUrl(),
                 QStringLiteral("http://persisted:7000"));
        QCOMPARE(ProjectConfig::instance().apiUrl(),
                 QStringLiteral("http://persisted:7000"));
        QCOMPARE(ProjectConfig::instance().sourceType(),
                 QStringLiteral("server"));
        QVERIFY(ProjectConfig::instance().smbHost().isEmpty());
    }

    void testCredentialIsolationAndQSettings() {
        auto *creds = new Credentials::MemoryCredentialStore();
        auto *svc = new Service::LocalServiceManager(this);
        SettingsController controller(nullptr, creds, svc, this);

        controller.setDraftSourceType(QStringLiteral("local"));
        controller.setDraftLocalPath(QStringLiteral("/tmp"));
        controller.setDraftToken(QStringLiteral("top-secret-token"));
        QVERIFY(controller.isDirty());
        controller.apply();
        QVERIFY(!controller.isDirty());

        QSettings s("isgaar_test", "Dopamine");
        QCOMPARE(s.value("Source/Mode").toString(), QStringLiteral("local"));
        QCOMPARE(s.value("Source/FolderPath").toString(),
                 QStringLiteral("/tmp"));
        QVERIFY(!s.contains("Source/Token"));
        QVERIFY(!s.contains("api"));
        QCOMPARE(creds->retrieveToken("api"),
                 QStringLiteral("top-secret-token"));
    }

    void testLocalServiceFolderValidation() {
        Service::LocalServiceManager mgr;
        QVERIFY(!mgr.start(QStringLiteral("/non_existent_folder_abc_123")));
        QCOMPARE(mgr.lastError(), QStringLiteral("folder_not_found"));
    }

    void testFirstRunCompletion() {
        SettingsController controller;
        controller.completeFirstRun();
        QVERIFY(!controller.isFirstRun());
        QSettings s("isgaar_test", "Dopamine");
        QVERIFY(s.value("FirstRun/Completed").toBool());
    }

    void testAppearanceDefaults() {
        SettingsController controller;
        QCOMPARE(controller.draftTheme(), QStringLiteral("roseglow"));
        QCOMPARE(controller.draftLanguage(), QStringLiteral("system"));
        QVERIFY(!controller.isDirty());
    }

    void testAppearancePersistsToXml() {
        SettingsController controller;
        controller.setDraftTheme(QStringLiteral("exoblur"));
        controller.setDraftLanguage(QStringLiteral("en"));
        QVERIFY(controller.isDirty());
        controller.apply();
        QVERIFY(!controller.isDirty());

        QFile file(SettingsStorage::appearanceFilePath());
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString xml = QString::fromUtf8(file.readAll());
        QVERIFY(xml.contains(QStringLiteral(
            "<theme>exoblur</theme>")));
        QVERIFY(xml.contains(QStringLiteral(
            "<language>en</language>")));

        SettingsController reloaded;
        QCOMPARE(reloaded.draftTheme(), QStringLiteral("exoblur"));
        QCOMPARE(reloaded.draftLanguage(), QStringLiteral("en"));
    }
};

QTEST_MAIN(TestSettingsController)
#include "test_settings_controller.moc"
