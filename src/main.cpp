/**
 * Location: src/main.cpp
 * Function: Native client bootstrap entrypoint configuring single-instance
 * messaging, registering controllers, and initializing the Qt Quick runtime.
 * All Rights Reserved isgaar
 */
#include <QApplication>
#include <QCommandLineParser>
#include <QFontDatabase>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTimer>
#include <cstdio>
#include "core/ApiClient.h"
#include "core/ProjectConfig.h"
#include "core/SingleInstance.h"
#include "core/audio/DspAudioBackend.h"
#include "core/credentials/FileCredentialStore.h"
#include "core/discovery/DiscoveryService.h"
#include "core/platform/PlatformHelper.h"
#include "features/interconnect/app/SessionPlaybackController.h"
#include "features/interconnect/infra/SessionPlaybackApiClient.h"
#include "features/playback/app/AudioSettingsController.h"
#include "features/playback/app/EqualizerController.h"
#include "features/playback/app/PlaybackController.h"
#include "features/settings/app/SettingsController.h"
#include "features/library/app/LibraryController.h"
#include "features/queue/app/QueueController.h"
#include "features/playlists/app/PlaylistController.h"
#include "shared/Theme.h"
#include "shared/colors/ColorManager.h"
#include "shared/i18n/StringManager.h"
#include <dopamine_desktop/src/bridge.cxxqt.h>
#include <dopamine_desktop/src/session/session_bridge.cxxqt.h>

int main(int argc, char *argv[]) {
    qputenv("QML_DISABLE_DISTANCEFIELD", "1");
    QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering);
    QQuickWindow::setDefaultAlphaBuffer(true);
    QApplication app(argc, argv);
    app.setApplicationName("Dopamine"); app.setOrganizationName("isgaar");
    app.setWindowIcon(QIcon(":/Dopamine/Core/src/shared/assets/app-icon.png"));
    const QString pf = ":/Dopamine/Core/src/shared/assets/fonts/";
    for (const auto &fn : {"LumpySansLight.ttf", "LumpySansRegular.ttf",
         "LumpySans-Medium.ttf", "LumpySansBold.ttf",
         "LumpySansHeavy.ttf", "lucide.ttf"})
        QFontDatabase::addApplicationFont(pf + fn);
    QFont f("LumpySans", 10); f.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(f);
    QCommandLineParser parser;
    parser.addOption({{"u", "uri"}, "URI", "uri"});
    parser.addPositionalArgument("uri", "dopamine:// handoff URI", "[uri]");
    parser.process(app);
    QString initialUri = parser.value("uri");
    if (initialUri.isEmpty()) {
        const QStringList positional = parser.positionalArguments();
        if (!positional.isEmpty()) initialUri = positional.first();
    }
    Dopamine::Core::SingleInstance singleInstance("dopamine_native");
    if (singleInstance.checkAndForward(initialUri)) return 0;
    singleInstance.startListening();
    using namespace Dopamine::Shared;
    auto &theme = Theme::instance();
    auto &strings = I18n::StringManager::instance();
    auto &colors = Colors::ColorManager::instance();
    auto &platform = Dopamine::Core::Platform::PlatformHelper::instance();
    auto &config = Dopamine::Core::ProjectConfig::instance();
    colors.initialize(); strings.initialize();
    dopamine::rust::CoreBridge rustBridge;
    rustBridge.setBaseUrl(config.apiUrl());
    QObject::connect(&rustBridge, &dopamine::rust::CoreBridge::statusChanged,
                     [&rustBridge]() {
                         const QByteArray status =
                             rustBridge.getStatus().toUtf8();
                         std::fprintf(stderr, "[rust] %s\n",
                                        status.constData());
                     });
    QObject::connect(&config, &Dopamine::Core::ProjectConfig::configChanged,
                     &rustBridge, [&config, &rustBridge]() {
                         rustBridge.setBaseUrl(config.apiUrl());
                     });
    Dopamine::Core::ApiClient apiClient;
    Dopamine::Core::Audio::DspAudioBackend audioBackend;
    Dopamine::Core::Discovery::DiscoveryService discoveryService;
    using namespace Dopamine::Features;
    Playback::App::PlaybackController playbackCtrl(&audioBackend);
    Playback::App::EqualizerController eqCtrl;
    auto syncEq = [&]() {
        std::vector<float> g;
        for (const auto &v : eqCtrl.bandGains())
            g.push_back(static_cast<float>(v.toDouble()));
        audioBackend.setEqualizer(g, static_cast<float>(eqCtrl.preamp()));
    };
    QObject::connect(&eqCtrl,
        &Playback::App::EqualizerController::paramsChanged, syncEq);
    syncEq();
    Playback::App::AudioSettingsController audioSettings;
    Dopamine::Core::Credentials::FileCredentialStore credentials;
    Settings::App::SettingsController settingsCtrl(&apiClient, &credentials);
    dopamine::rust::SessionBridge sessionBridge;
    QObject::connect(&sessionBridge,
        &dopamine::rust::SessionBridge::serverAdopted,
        &settingsCtrl, &Settings::App::SettingsController::adoptRemoteServer);
    QObject::connect(&sessionBridge,
        &dopamine::rust::SessionBridge::bearerTokenChanged,
        &apiClient, [&apiClient](const QString &token) {
            apiClient.setBearerToken(token);
        });
    QString pendingUri = initialUri;
    bool qmlReady = false;
    sessionBridge.initialize();
    Library::Infra::LibraryApiClient libApi(&apiClient);
    Library::App::LibraryController libCtrl(&libApi);
    Queue::Infra::QueueApiClient qApi(&apiClient);
    Queue::App::QueueController queueCtrl(&qApi);
    Interconnect::Infra::SessionPlaybackApiClient sessionPlaybackApi(
        &apiClient);
    Interconnect::App::SessionPlaybackController sessionPlayback(
        &sessionPlaybackApi, &queueCtrl);
    Playlists::Infra::PlaylistApiClient plApi(&apiClient);
    Playlists::App::PlaylistController plCtrl(&plApi);
    QObject::connect(&sessionBridge,
        &dopamine::rust::SessionBridge::serverAdopted,
        &libCtrl, [&libCtrl](const QString &) {
            libCtrl.loadTracks(1);
        });
    QObject::connect(&sessionBridge,
        &dopamine::rust::SessionBridge::loginSucceeded,
        &libCtrl, [&libCtrl]() {
            libCtrl.loadTracks(1);
        });
    QObject::connect(&queueCtrl,
        &Queue::App::QueueController::playTrackRequested,
        &playbackCtrl, [&playbackCtrl](int id, auto ti, auto ar,
                                       auto al, auto path, int d) {
            playbackCtrl.playLibraryTrack(id, ti, ar, al, path, d);
        });
    QObject::connect(&queueCtrl,
        &Queue::App::QueueController::sessionTrackRequested,
        &playbackCtrl, [&playbackCtrl](int id, auto ti, auto ar, auto al,
                                       auto path, int d, int pos) {
            playbackCtrl.loadLibraryTrack(id, ti, ar, al, path, d, pos);
        });
    QObject::connect(&sessionBridge,
        &dopamine::rust::SessionBridge::serverAdopted,
        &sessionPlayback, [&sessionPlayback](const QString &) {
            sessionPlayback.start();
        });
    QObject::connect(&sessionBridge,
        &dopamine::rust::SessionBridge::isLoggedInChanged,
        &sessionPlayback, [&sessionBridge, &sessionPlayback]() {
            if (sessionBridge.getIsLoggedIn())
                sessionPlayback.start();
            else
                sessionPlayback.stop();
        });
    if (sessionBridge.getIsLoggedIn()) sessionPlayback.start();

    discoveryService.startDiscovery();
    QQmlApplicationEngine engine;
    QObject::connect(&singleInstance,
        &Dopamine::Core::SingleInstance::messageReceived,
        &app, [&pendingUri, &qmlReady, &sessionBridge, &engine]
        (const QString &uri) {
            const auto objects = engine.rootObjects();
            if (!objects.isEmpty()) {
                if (auto *w = qobject_cast<QQuickWindow *>(objects.first())) {
                    w->show(); w->raise(); w->requestActivate();
                }
            }
            if (qmlReady) sessionBridge.handleUri(uri);
            else pendingUri = uri;
        });
    auto *ctx = engine.rootContext();
    for (auto [k, v] : {
        std::pair{"Theme", (QObject*)&theme}, {"Strings", &strings},
        {"Colors", &colors}, {"Platform", &platform}, {"config", &config},
        {"playbackController", &playbackCtrl},
        {"discoveryService", &discoveryService},
        {"equalizerController", &eqCtrl}, {"audioSettings", &audioSettings},
        {"settingsController", &settingsCtrl},
        {"sessionController", &sessionBridge},
        {"libraryController", (QObject*)&libCtrl},
        {"queueController", (QObject*)&queueCtrl},
        {"sessionPlaybackController", (QObject*)&sessionPlayback},
        {"playlistController", (QObject*)&plCtrl},
        {"rustBridge", (QObject*)&rustBridge}})
        ctx->setContextProperty(QString::fromLatin1(k), v);
    rustBridge.checkConnection();

    const QUrl url(QStringLiteral("qrc:/Dopamine/Core/src/Main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app,
        [url, &app, &platform, &sessionBridge, &qmlReady, &pendingUri]
        (QObject *o, const QUrl &u) {
            if (u != url) return;
            if (!o) { QCoreApplication::exit(-1); return; }
            if (auto *w = qobject_cast<QQuickWindow *>(o)) {
                platform.setupWindow(w);
                w->show();
            }
            qmlReady = true;
            QTimer::singleShot(0, &app, [&sessionBridge, &pendingUri]() {
                if (pendingUri.isEmpty()) return;
                const QString uri = pendingUri;
                pendingUri.clear();
                sessionBridge.handleUri(uri);
            });
        }, Qt::QueuedConnection);
    engine.load(url);
    return app.exec();
}
