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
#include "core/platform/media/PlatformMediaService.h"
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
        audioBackend.setLimiterEnabled(eqCtrl.limiter());
        audioBackend.setStabilizerEnabled(eqCtrl.agc());
        audioBackend.setEffects1(
            eqCtrl.zBassActive() ? static_cast<float>(eqCtrl.zBass() * 100.0) : 0.0f,
            eqCtrl.zMidsActive() ? static_cast<float>(eqCtrl.zMids() * 100.0) : 0.0f,
            eqCtrl.zTrebleActive() ? static_cast<float>(eqCtrl.zTreble() * 100.0) : 0.0f,
            0.0f,
            eqCtrl.compressorActive() ? static_cast<float>(eqCtrl.compressor() * 100.0) : 0.0f,
            static_cast<float>(eqCtrl.balance()));
        audioBackend.setEffects2(
            eqCtrl.echoActive(), static_cast<float>(eqCtrl.echo() * 100.0),
            eqCtrl.autoWahActive(), static_cast<float>(eqCtrl.autoWah() * 100.0),
            eqCtrl.reverbActive(), static_cast<float>(eqCtrl.reverb() * 100.0),
            eqCtrl.flanger());
    };
    QObject::connect(&eqCtrl,
        &Playback::App::EqualizerController::paramsChanged, syncEq);
    syncEq();
    Playback::App::AudioSettingsController audioSettings;
    auto syncAudioSettings = [&]() {
        audioBackend.setCrossfadeMs(
            static_cast<int>(audioSettings.crossfadeSec() * 1000.0));
    };
    QObject::connect(&audioSettings,
        &Playback::App::AudioSettingsController::settingsChanged,
        syncAudioSettings);
    syncAudioSettings();
    Dopamine::Core::Credentials::FileCredentialStore credentials;
    Settings::App::SettingsController settingsCtrl(&apiClient, &credentials);
    dopamine::rust::SessionBridge sessionBridge;
    QObject::connect(&sessionBridge,
        &dopamine::rust::SessionBridge::serverAdopted,
        &settingsCtrl, &Settings::App::SettingsController::adoptRemoteServer);
    QObject::connect(&sessionBridge,
        &dopamine::rust::SessionBridge::bearerTokenChanged,
        &apiClient, [&apiClient, &audioBackend](const QString &token) {
            apiClient.setBearerToken(token);
            audioBackend.setBearerToken(token.toUtf8());
        });
    QString pendingUri = initialUri;
    bool qmlReady = false;
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
    QObject::connect(&audioBackend,
        &Dopamine::Core::Audio::IAudioBackend::trackFinished,
        &queueCtrl, &Queue::App::QueueController::next);
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
    QObject::connect(&settingsCtrl,
        &Settings::App::SettingsController::rescanTriggered,
        &libCtrl, [&libCtrl]() {
            libCtrl.loadTracks(1);
        });
    QObject::connect(&queueCtrl,
        &Queue::App::QueueController::playTrackRequested,
        &playbackCtrl, [&playbackCtrl, &queueCtrl, &audioBackend,
                        &sessionPlayback]
        (int id, auto ti, auto ar, auto al, auto path, int d) {
            if (sessionPlayback.isControllingAndroid()) {
                playbackCtrl.loadLibraryTrack(id, ti, ar, al, path, d, 0);
                sessionPlayback.sendRemoteCommandToAndroid(
                    id, 0, true, playbackCtrl.volume());
                return;
            }
            playbackCtrl.playLibraryTrack(id, ti, ar, al, path, d);
            const QString nextPath = queueCtrl.nextTrackPath();
            audioBackend.setNext(nextPath.isEmpty() ? QUrl{} : QUrl(nextPath));
        });
    QObject::connect(&queueCtrl,
        &Queue::App::QueueController::sessionTrackRequested,
        &playbackCtrl, [&playbackCtrl](int id, auto ti, auto ar, auto al,
                                       auto path, int d, int pos) {
            playbackCtrl.loadLibraryTrack(id, ti, ar, al, path, d, pos);
        });
    QObject::connect(&sessionPlayback,
        &Interconnect::App::SessionPlaybackController::takeoverDesktopRequested,
        &playbackCtrl, &Playback::App::PlaybackController::play);
    QObject::connect(&sessionPlayback,
        &Interconnect::App::SessionPlaybackController::pauseLocalRequested,
        &playbackCtrl, &Playback::App::PlaybackController::pause);
    QObject::connect(&sessionPlayback,
        &Interconnect::App::SessionPlaybackController::remoteProgressUpdated,
        &playbackCtrl, [&playbackCtrl](int posSec, int volPct, bool) {
            playbackCtrl.syncRemoteProgress(posSec, volPct);
        });
    QObject::connect(&sessionPlayback,
        &Interconnect::App::SessionPlaybackController::remoteSeekCommandReceived,
        &playbackCtrl, &Playback::App::PlaybackController::seekFromRemote);
    QObject::connect(&sessionPlayback,
        &Interconnect::App::SessionPlaybackController::remoteVolumeCommandReceived,
        &playbackCtrl, &Playback::App::PlaybackController::setVolumeFromRemote);
    int lastPublishedSec = -1;
    auto publishLocalState = [&]() {
        sessionPlayback.syncLocalPlayback(
            playbackCtrl.trackId(), playbackCtrl.position(),
            playbackCtrl.isPlaying(), playbackCtrl.volume());
    };
    QObject::connect(&playbackCtrl,
        &Playback::App::PlaybackController::playStateChanged,
        &sessionPlayback, publishLocalState);
    QObject::connect(&playbackCtrl,
        &Playback::App::PlaybackController::trackChanged,
        &sessionPlayback, publishLocalState);
    QObject::connect(&playbackCtrl,
        &Playback::App::PlaybackController::positionChanged,
        &sessionPlayback, [&]() {
            if (!playbackCtrl.isPlaying()) return;
            const int pos = playbackCtrl.position();
            if (qAbs(pos - lastPublishedSec) >= 2) {
                lastPublishedSec = pos;
                publishLocalState();
            }
        });
    QObject::connect(&playbackCtrl,
        &Playback::App::PlaybackController::volumeChanged,
        &sessionPlayback, [&]() {
            publishLocalState();
        });
    const bool isDynamicAccent = settingsCtrl.getSetting(
        QStringLiteral("Appearance/dynamicAccent"), true).toBool();
    colors.setDynamicAccentEnabled(isDynamicAccent);
    QObject::connect(&playbackCtrl,
        &Playback::App::PlaybackController::trackChanged,
        &colors, [&playbackCtrl, &colors]() {
            colors.updateTrackCover(playbackCtrl.trackCover(), playbackCtrl.trackId());
        });
    QObject::connect(&playbackCtrl,
        &Playback::App::PlaybackController::positionChanged,
        &sessionPlayback, [&playbackCtrl, &lastPublishedSec, publishLocalState]() {
            if (!playbackCtrl.isPlaying()) return;
            const int pos = playbackCtrl.position();
            if (pos != lastPublishedSec) {
                lastPublishedSec = pos;
                publishLocalState();
            }
        });
    QObject::connect(&playbackCtrl,
        &Playback::App::PlaybackController::userSeeked,
        &sessionPlayback, [&playbackCtrl, &sessionPlayback, publishLocalState]
        (int seconds) {
            if (sessionPlayback.isControllingAndroid()) {
                sessionPlayback.sendRemoteCommandToAndroid(
                    playbackCtrl.trackId(), seconds,
                    sessionPlayback.remotePlaying(), playbackCtrl.volume());
            } else {
                publishLocalState();
            }
        });
    QObject::connect(&playbackCtrl,
        &Playback::App::PlaybackController::userVolumeChanged,
        &sessionPlayback, [&playbackCtrl, &sessionPlayback, publishLocalState]
        (int vol) {
            if (sessionPlayback.isControllingAndroid()) {
                sessionPlayback.sendRemoteCommandToAndroid(
                    playbackCtrl.trackId(), playbackCtrl.position(),
                    sessionPlayback.remotePlaying(), vol);
            } else {
                publishLocalState();
            }
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
    sessionBridge.initialize();
    if (!config.apiUrl().isEmpty()) {
        libCtrl.loadTracks(1);
        sessionPlayback.start();
    }

    discoveryService.startDiscovery();
    Dopamine::Core::Platform::PlatformMediaService &platformMedia =
        Dopamine::Core::Platform::PlatformMediaService::instance();
    platformMedia.initialize(&playbackCtrl, &queueCtrl);

    QQmlApplicationEngine engine;
    QObject::connect(&platformMedia,
        &Dopamine::Core::Platform::PlatformMediaService::raiseRequested,
        &engine, [&engine]() {
            const auto objects = engine.rootObjects();
            if (!objects.isEmpty()) {
                if (auto *w = qobject_cast<QQuickWindow *>(objects.first())) {
                    w->show(); w->raise(); w->requestActivate();
                }
            }
        });
    QObject::connect(&platformMedia,
        &Dopamine::Core::Platform::PlatformMediaService::quitRequested,
        &app, &QCoreApplication::quit);
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
