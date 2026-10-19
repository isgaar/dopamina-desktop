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
#include "core/ApiClient.h"
#include "core/ProjectConfig.h"
#include "core/SingleInstance.h"
#include "core/audio/DspAudioBackend.h"
#include "core/discovery/DiscoveryService.h"
#include "core/platform/PlatformHelper.h"
#include "features/playback/app/AudioSettingsController.h"
#include "features/playback/app/EqualizerController.h"
#include "features/playback/app/PlaybackController.h"
#include "features/settings/app/SettingsController.h"
#include "features/stations/app/StationController.h"
#include "features/stations/infra/StationHttpService.h"
#include "features/library/app/LibraryController.h"
#include "features/queue/app/QueueController.h"
#include "features/playlists/app/PlaylistController.h"
#include "shared/Theme.h"
#include "shared/colors/ColorManager.h"
#include "shared/i18n/StringManager.h"

int main(int argc, char *argv[]) {
    qputenv("QML_DISABLE_DISTANCEFIELD", "1");
    QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering);
    QQuickWindow::setDefaultAlphaBuffer(true);
    QApplication app(argc, argv);
    app.setApplicationName("Dopamine"); app.setOrganizationName("isgaar");
    app.setWindowIcon(QIcon(":/Dopamine/Core/src/shared/assets/app-icon.png"));
    const QString pf = ":/Dopamine/Core/src/shared/assets/fonts/";
    for (const auto &fn : {"LumpySansLight.ttf", "LumpySansRegular.ttf",
         "LumpySans-Medium.ttf", "LumpySansBold.ttf", "lucide.ttf"})
        QFontDatabase::addApplicationFont(pf + fn);
    QFont f("LumpySans", 10); f.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(f);
    QCommandLineParser parser;
    parser.addOption({{"u", "uri"}, "URI", "uri"});
    parser.process(app);
    Dopamine::Core::SingleInstance singleInstance("dopamine_native");
    if (singleInstance.checkAndForward(parser.value("uri"))) return 0;
    singleInstance.startListening();
    using namespace Dopamine::Shared;
    auto &theme = Theme::instance();
    auto &strings = I18n::StringManager::instance();
    auto &colors = Colors::ColorManager::instance();
    auto &platform = Dopamine::Core::Platform::PlatformHelper::instance();
    auto &config = Dopamine::Core::ProjectConfig::instance();
    colors.initialize(); strings.initialize();
    Dopamine::Core::ApiClient apiClient;
    Dopamine::Core::Audio::DspAudioBackend audioBackend;
    Dopamine::Core::Discovery::DiscoveryService discoveryService;
    using namespace Dopamine::Features;
    Playback::App::PlaybackController playbackCtrl(&apiClient, &audioBackend);
    Stations::Infra::StationHttpService httpService(&apiClient);
    Stations::App::StationController stationCtrl(nullptr, &httpService);
    stationCtrl.setAudioBackend(&audioBackend);
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
    Settings::App::SettingsController settingsCtrl(&apiClient);
    Library::Infra::LibraryApiClient libApi(&apiClient);
    Library::App::LibraryController libCtrl(&libApi);
    Queue::Infra::QueueApiClient qApi(&apiClient);
    Queue::App::QueueController queueCtrl(&qApi);
    Playlists::Infra::PlaylistApiClient plApi(&apiClient);
    Playlists::App::PlaylistController plCtrl(&plApi);
    QObject::connect(&queueCtrl,
        &Queue::App::QueueController::playTrackRequested,
        &playbackCtrl, [&playbackCtrl](int id, auto ti, auto ar,
                                       auto al, int d) {
            playbackCtrl.playLibraryTrack(id, ti, ar, al, "", d);
        });

    discoveryService.startDiscovery();
    QQmlApplicationEngine engine;
    auto *ctx = engine.rootContext();
    for (auto [k, v] : {
        std::pair{"Theme", (QObject*)&theme}, {"Strings", &strings},
        {"Colors", &colors}, {"Platform", &platform}, {"config", &config},
        {"playbackController", &playbackCtrl},
        {"discoveryService", &discoveryService},
        {"stationController", &stationCtrl}, {"controller", &stationCtrl},
        {"equalizerController", &eqCtrl}, {"audioSettings", &audioSettings},
        {"settingsController", &settingsCtrl},
        {"libraryController", (QObject*)&libCtrl},
        {"queueController", (QObject*)&queueCtrl},
        {"playlistController", (QObject*)&plCtrl}})
        ctx->setContextProperty(QString::fromLatin1(k), v);

    const QUrl url(QStringLiteral("qrc:/Dopamine/Core/src/Main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app,
        [url](QObject *o, const QUrl &u) {
            if (!o && u == url) QCoreApplication::exit(-1);
        }, Qt::QueuedConnection);
    engine.load(url);
    auto *rootObj = engine.rootObjects().value(0);
    if (auto *w = qobject_cast<QQuickWindow *>(rootObj)) {
        platform.setupWindow(w); w->show();
    }
    return app.exec();
}
