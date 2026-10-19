/**
 * Location: tests/test_smb_storage.cpp
 * Function: Unit tests for SMB and local storage providers, atomic descriptor
 * creation, and secure SMB credentials isolation in ICredentialStore.
 * All Rights Reserved isgaar
 */
#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include "core/ProjectConfig.h"
#include "core/credentials/MemoryCredentialStore.h"
#include "core/service/LocalServiceManager.h"
#include "core/storage/LocalStorageProvider.h"
#include "core/storage/SmbStorageProvider.h"
#include "features/settings/app/SettingsController.h"
#include "features/settings/infra/SettingsStorage.h"
#include "features/settings/infra/SourceProbe.h"

using namespace Dopamine::Core::Storage;
using namespace Dopamine::Core::Credentials;
using namespace Dopamine::Features::Settings::App;
using namespace Dopamine::Features::Settings::Infra;

class TestSmbStorage : public QObject {
    Q_OBJECT

private slots:
    void testLocalStorageProviderDescriptor() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        LocalStorageProvider prov(tmp.path());
        QVERIFY(prov.probe());
        QVERIFY(prov.ensureServerDescriptor(
            QStringLiteral("srv-uuid-1"), QStringLiteral("TestLib")));
        QVERIFY(prov.existsRootFile(QStringLiteral(".dopamine_server.json")));
    }

    void testSmbStorageProviderWithOverride() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        SmbStorageProvider prov(QStringLiteral("192.168.0.2"),
                                QStringLiteral("datos"),
                                QStringLiteral("FLAC"),
                                QStringLiteral("ismael"),
                                tmp.path());
        QVERIFY(prov.probe());
        QVERIFY(prov.connect());
        QVERIFY(prov.ensureServerDescriptor(
            QStringLiteral("srv-smb-2"), QStringLiteral("DatosMusic")));
        QVERIFY(prov.existsRootFile(QStringLiteral(".dopamine_server.json")));
    }

    void testFirstRunSmbUriConfigurationAndCredentials() {
        SettingsStorage::setTestScope(true);
        QSettings s(QStringLiteral("isgaar_test"), QStringLiteral("Dopamine"));
        s.clear();
        auto *creds = new MemoryCredentialStore();
        auto *svc = new Dopamine::Core::Service::LocalServiceManager(this);
        SettingsController controller(nullptr, creds, svc, this);

        QVERIFY(controller.isFirstRun());
        controller.setDraftSourceType(QStringLiteral("smb"));
        controller.setDraftSmbHost(QStringLiteral("192.168.0.2"));
        controller.setDraftSmbShare(QStringLiteral("datos"));
        controller.setDraftSmbUser(QStringLiteral("ismael"));
        controller.setDraftSmbPassword(QStringLiteral("secret123"));
        controller.setDraftSaveCredentials(true);
        controller.completeFirstRun();

        QVERIFY(!controller.isFirstRun());
        QSettings check(
            QStringLiteral("isgaar_test"), QStringLiteral("Dopamine"));
        QCOMPARE(check.value(QStringLiteral("Source/SmbHost")).toString(),
                 QStringLiteral("192.168.0.2"));
        QCOMPARE(check.value(QStringLiteral("Source/SmbShare")).toString(),
                 QStringLiteral("datos"));
        QCOMPARE(check.value(QStringLiteral("Source/SmbUser")).toString(),
                 QStringLiteral("ismael"));
        QVERIFY(check.value(QStringLiteral("Source/SaveCredentials")).toBool());
        QVERIFY(!check.contains(QStringLiteral("Source/SmbPassword")));
        QCOMPARE(creds->retrieveToken(QStringLiteral("smb")),
                 QStringLiteral("secret123"));

        controller.setDraftSaveCredentials(false);
        controller.apply();
        QVERIFY(creds->retrieveToken(QStringLiteral("smb")).isEmpty());
        SettingsStorage::setTestScope(false);
    }

    void testProbeSmbSanitizesUri() {
        QString res = Dopamine::Features::Settings::Infra::SourceProbe::
            probeSmb(QStringLiteral("smb://192.168.0.2/datos/"), 1);
        QCOMPARE(res, QStringLiteral("unreachable"));
    }
};

QTEST_MAIN(TestSmbStorage)
#include "test_smb_storage.moc"
