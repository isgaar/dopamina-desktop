/**
 * Location: src/features/settings/ui/panes/SystemSettingsPane.qml
 * Function: System settings pane configuring application lifecycle options,
 * headphone connect/disconnect, audio focus loss, bluetooth events, volume hotkeys,
 * zero-volume pausing, power-saving modes, and runtime environment versions.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui"
import "root:/src/shared/ui/settings"

SettingsPane {
    id: root
    title: Strings.get("settings_system_title")
    description: Strings.get("settings_system_desc")

    ColumnLayout {
        Layout.fillWidth: true
        spacing: Theme.spaceMd

        SettingsSwitchRow {
            title: Strings.get("settings_resume_call")
            subtitle: Strings.get("settings_resume_call_desc")
            checked: settingsController ? settingsController.getSetting("System/resumeAfterCall", true) : true
            onToggled: isChecked => {
                if (settingsController) settingsController.setSetting("System/resumeAfterCall", isChecked)
            }
        }

        SettingsSwitchRow {
            title: Strings.get("settings_pause_focus_title")
            subtitle: Strings.get("settings_pause_focus_desc")
            checked: settingsController ? settingsController.getSetting("System/pauseOnFocusLoss", true) : true
            onToggled: isChecked => {
                if (settingsController) settingsController.setSetting("System/pauseOnFocusLoss", isChecked)
            }
        }

        SettingsSwitchRow {
            title: Strings.get("settings_headphone_unplug_title")
            subtitle: Strings.get("settings_headphone_unplug_desc")
            checked: settingsController ? settingsController.getSetting("System/headphoneUnplugPause", true) : true
            onToggled: isChecked => {
                if (settingsController) settingsController.setSetting("System/headphoneUnplugPause", isChecked)
            }
        }

        SettingsSwitchRow {
            title: Strings.get("settings_headphone_plug_title")
            subtitle: Strings.get("settings_headphone_plug_desc")
            checked: settingsController ? settingsController.getSetting("System/headphonePlugResume", false) : false
            onToggled: isChecked => {
                if (settingsController) settingsController.setSetting("System/headphonePlugResume", isChecked)
            }
        }

        SettingsSwitchRow {
            title: Strings.get("settings_bt_connect_title")
            subtitle: Strings.get("settings_bt_connect_desc")
            checked: settingsController ? settingsController.getSetting("System/bluetoothConnectResume", true) : true
            onToggled: isChecked => {
                if (settingsController) settingsController.setSetting("System/bluetoothConnectResume", isChecked)
            }
        }

        SettingsSwitchRow {
            title: Strings.get("settings_vol_keys")
            subtitle: Strings.get("settings_vol_keys_desc")
            checked: settingsController ? settingsController.getSetting("System/volumeKeysEnabled", true) : true
            onToggled: isChecked => {
                if (settingsController) settingsController.setSetting("System/volumeKeysEnabled", isChecked)
            }
        }

        SettingsSwitchRow {
            title: Strings.get("settings_vol_zero_title")
            subtitle: Strings.get("settings_vol_zero_desc")
            checked: settingsController ? settingsController.getSetting("System/pauseOnVolumeZero", false) : false
            onToggled: isChecked => {
                if (settingsController) settingsController.setSetting("System/pauseOnVolumeZero", isChecked)
            }
        }

        SettingsSwitchRow {
            title: Strings.get("settings_powersaving")
            subtitle: Strings.get("settings_powersaving_desc")
            checked: settingsController ? settingsController.getSetting("System/powerSaving", false) : false
            onToggled: isChecked => {
                if (settingsController) settingsController.setSetting("System/powerSaving", isChecked)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            AppButton {
                text: Strings.get("settings_check_updates")
                variant: "secondary"
            }
            Item { Layout.fillWidth: true }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.borderDark
        }

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: Strings.get("settings_app_version") + ": " +
                      (settingsController ? settingsController.version : "2.0")
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSm
                color: Theme.textMuted
            }
            Item { Layout.fillWidth: true }
            Text {
                text: Strings.get("settings_qt_version") + ": " +
                      (settingsController ? settingsController.qtVersion : "")
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSm
                color: Theme.textMuted
            }
        }

        Text {
            text: Strings.get("settings_all_rights")
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSm
            color: Theme.textMuted
        }
    }
}
