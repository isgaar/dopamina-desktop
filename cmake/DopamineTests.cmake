# Location: cmake/DopamineTests.cmake
# Function: Test suite executable targets and ctest registration for settings,
# playback, audio DSP, equalizer, library, fault scenarios, and error systems.
# All Rights Reserved isgaar

enable_testing()

add_executable(test_settings_controller tests/test_settings_controller.cpp
    src/features/settings/app/SettingsController.cpp
    src/features/settings/infra/SettingsStorage.cpp
    src/features/settings/infra/SourceProbe.cpp
    src/core/credentials/MemoryCredentialStore.cpp
    src/core/service/LocalServiceManager.cpp
    src/core/ApiClient.cpp src/core/ProjectConfig.cpp
    src/core/platform/PlatformHelper.cpp src/shared/i18n/StringManager.cpp
    src/shared/colors/ColorManager.cpp)
target_link_libraries(test_settings_controller PRIVATE
    Qt6::Core Qt6::Network Qt6::Quick Qt6::Test)
target_include_directories(test_settings_controller PRIVATE src)
add_test(NAME SettingsControllerTest COMMAND test_settings_controller)

add_executable(test_smb_storage tests/test_smb_storage.cpp
    src/core/storage/SmbStorageProvider.cpp
    src/core/storage/LocalStorageProvider.cpp
    src/core/credentials/MemoryCredentialStore.cpp
    src/features/settings/infra/SettingsStorage.cpp
    src/features/settings/infra/SourceProbe.cpp
    src/features/settings/app/SettingsController.cpp
    src/core/ApiClient.cpp src/core/ProjectConfig.cpp
    src/core/service/LocalServiceManager.cpp
    src/core/platform/PlatformHelper.cpp src/shared/i18n/StringManager.cpp
    src/shared/colors/ColorManager.cpp)
target_link_libraries(test_smb_storage PRIVATE
    Qt6::Core Qt6::Network Qt6::Quick Qt6::Test)
target_include_directories(test_smb_storage PRIVATE src)
add_test(NAME SmbStorageTest COMMAND test_smb_storage)

add_executable(test_playback_controller tests/test_playback_controller.cpp
    src/core/audio/IAudioBackend.h
    src/features/playback/app/PlaybackController.cpp
    src/features/playback/app/EqualizerController.cpp
    src/features/playback/domain/EqualizerState.cpp
    src/features/playback/domain/LrcParser.cpp
    src/core/ApiClient.cpp src/core/ProjectConfig.cpp)
target_link_libraries(test_playback_controller PRIVATE
    Qt6::Core Qt6::Network Qt6::Quick Qt6::Test)
target_include_directories(test_playback_controller PRIVATE src)
add_test(NAME PlaybackControllerTest COMMAND test_playback_controller)

add_executable(test_audio_dsp tests/test_audio_dsp.cpp
    src/core/audio/dsp/GainStage.cpp
    src/core/audio/automix/CrossfadeMixer.cpp
    src/core/audio/dsp/LoudnessGain.cpp)
target_link_libraries(test_audio_dsp PRIVATE
    Qt6::Core Qt6::Multimedia Qt6::Test dopamine_desktop)
target_include_directories(test_audio_dsp PRIVATE src)
add_test(NAME AudioDspTest COMMAND test_audio_dsp)

add_executable(test_equalizer tests/test_equalizer.cpp
    src/features/playback/app/EqualizerController.cpp
    src/features/playback/domain/EqualizerState.cpp)
target_link_libraries(test_equalizer PRIVATE Qt6::Core Qt6::Test dopamine_desktop)
target_include_directories(test_equalizer PRIVATE src)
add_test(NAME EqualizerTest COMMAND test_equalizer)

add_executable(test_session_playback tests/test_session_playback.cpp
    src/features/interconnect/infra/SessionPlaybackApiClient.cpp
    src/features/interconnect/app/SessionPlaybackController.cpp
    src/features/queue/infra/QueueApiClient.cpp
    src/features/queue/app/QueueController.cpp
    src/core/ApiClient.cpp src/core/ProjectConfig.cpp)
target_link_libraries(test_session_playback PRIVATE
    Qt6::Core Qt6::Network Qt6::Test)
target_include_directories(test_session_playback PRIVATE src)
add_test(NAME SessionPlaybackTest COMMAND test_session_playback)

add_executable(test_player_features tests/test_player_features.cpp
    tests/faults/FakeApiServer.cpp
    src/features/library/infra/LibraryApiClient.cpp
    src/features/library/infra/RecursiveDirectoryScanner.cpp
    src/features/library/app/LibraryController.cpp
    src/core/storage/SmbStorageProvider.cpp
    src/features/queue/infra/QueueApiClient.cpp
    src/features/queue/app/QueueController.cpp
    src/features/playlists/infra/PlaylistApiClient.cpp
    src/features/playlists/app/PlaylistController.cpp
    src/core/ApiClient.cpp src/core/ProjectConfig.cpp)
target_link_libraries(test_player_features PRIVATE
    Qt6::Core Qt6::Network Qt6::Test)
target_include_directories(test_player_features PRIVATE src)
add_test(NAME PlayerFeaturesTest COMMAND test_player_features)

add_executable(test_transition_planner tests/test_transition_planner.cpp
    src/core/audio/automix/TransitionPlanner.cpp)
target_link_libraries(test_transition_planner PRIVATE Qt6::Core Qt6::Test)
target_include_directories(test_transition_planner PRIVATE src)
add_test(NAME TransitionPlannerTest COMMAND test_transition_planner)

add_executable(test_error_system tests/test_error_system.cpp
    src/core/errors/ErrorCode.cpp src/core/errors/Redactor.cpp
    src/core/logging/JsonLogger.cpp src/core/platform/UiWatchdog.cpp
    src/core/diagnostics/SystemDoctor.cpp src/core/connection/ProbeRunner.cpp
    src/core/ApiClient.cpp src/core/ProjectConfig.cpp
    src/core/storage/SmbStorageProvider.cpp
    src/core/storage/LocalStorageProvider.cpp
    src/core/credentials/MemoryCredentialStore.cpp
    src/core/platform/PlatformHelper.cpp src/shared/i18n/StringManager.cpp)
target_link_libraries(test_error_system PRIVATE
    Qt6::Core Qt6::Network Qt6::Quick Qt6::Test)
target_include_directories(test_error_system PRIVATE src)
add_test(NAME ErrorSystemTest COMMAND test_error_system)

add_executable(test_fault_scenarios tests/test_fault_scenarios.cpp
    tests/faults/FakeApiServer.cpp src/core/connection/ConnectionService.cpp
    src/core/connection/ProbeRunner.cpp
    src/features/library/infra/LibraryApiClient.cpp
    src/core/errors/ErrorCode.cpp src/core/errors/AppError.cpp
    src/core/errors/Redactor.cpp src/core/logging/JsonLogger.cpp
    src/core/ApiClient.cpp src/core/ProjectConfig.cpp
    src/core/storage/SmbStorageProvider.cpp
    src/core/storage/LocalStorageProvider.cpp
    src/core/credentials/MemoryCredentialStore.cpp
    src/core/platform/PlatformHelper.cpp src/shared/i18n/StringManager.cpp)
target_link_libraries(test_fault_scenarios PRIVATE
    Qt6::Core Qt6::Network Qt6::Quick Qt6::Test Qt6::Concurrent)
target_include_directories(test_fault_scenarios PRIVATE src)
add_test(NAME FaultScenariosTest COMMAND test_fault_scenarios)

add_executable(test_ambiguity_daemon tests/test_ambiguity_daemon.cpp
    src/core/daemon/AmbiguityDaemon.cpp src/core/daemon/AmbiguitySupervisor.cpp)
target_link_libraries(test_ambiguity_daemon PRIVATE Qt6::Core Qt6::Test)
target_include_directories(test_ambiguity_daemon PRIVATE src)
add_test(NAME AmbiguityDaemonTest COMMAND test_ambiguity_daemon)

if(TARGET Qt6::DBus)
    add_executable(test_platform_media tests/test_platform_media.cpp
        src/core/audio/IAudioBackend.h
        src/core/platform/media/mpris/MprisPlayerService.cpp
        src/core/platform/media/mpris/MprisPlayerAdaptor.cpp
        src/core/platform/media/mpris/MprisRootAdaptor.cpp
        src/core/platform/media/PlatformMediaService.cpp
        src/features/playback/app/PlaybackController.cpp
        src/features/playback/app/EqualizerController.cpp
        src/features/playback/domain/EqualizerState.cpp
        src/features/playback/domain/LrcParser.cpp
        src/features/queue/app/QueueController.cpp
        src/features/queue/infra/QueueApiClient.cpp
        src/core/ApiClient.cpp src/core/ProjectConfig.cpp)
    target_link_libraries(test_platform_media PRIVATE Qt6::Core Qt6::Test Qt6::Network Qt6::DBus)
    target_compile_definitions(test_platform_media PRIVATE DOPAMINE_HAS_DBUS)
    target_include_directories(test_platform_media PRIVATE src)
    add_test(NAME PlatformMediaTest COMMAND test_platform_media)
endif()
