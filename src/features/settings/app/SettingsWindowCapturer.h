/**
 * Location: src/features/settings/app/SettingsWindowCapturer.h
 * Function: Offscreen rendering diagnostics utility capturing screenshots
 * of SettingsWindow panes and modal dialogs for UI-1 acceptance criteria.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_SETTINGS_APP_SETTINGSWINDOWCAPTURER_H
#define DOPAMINE_FEATURES_SETTINGS_APP_SETTINGSWINDOWCAPTURER_H

#include <QString>

class QQmlApplicationEngine;

namespace Dopamine::Features::Settings::App {

bool captureSettingsPanels(QQmlApplicationEngine *engine,
                           const QString &outputDir);

} // namespace Dopamine::Features::Settings::App

#endif // DOPAMINE_FEATURES_SETTINGS_APP_SETTINGSWINDOWCAPTURER_H
