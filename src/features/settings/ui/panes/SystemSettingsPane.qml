/**
 * Location: src/features/settings/ui/panes/SystemSettingsPane.qml
 * Function: System settings pane configuring application lifecycle options,
 * volume hotkeys, power-saving modes, and runtime environment versions.
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
            checked: true
        }

        SettingsSwitchRow {
            title: Strings.get("settings_vol_keys")
            checked: false
        }

        SettingsSwitchRow {
            title: Strings.get("settings_powersaving")
            checked: false
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
