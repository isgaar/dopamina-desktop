/**
 * Location: src/features/settings/ui/panes/CoversSettingsPane.qml
 * Function: Covers settings pane managing album artwork download preferences,
 * synchronized lyrics retrieval, and image compression quality.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui"
import "root:/src/shared/ui/settings"

SettingsPane {
    id: root
    title: Strings.get("settings_covers_title")
    description: Strings.get("settings_covers_desc")

    ColumnLayout {
        Layout.fillWidth: true
        spacing: Theme.spaceMd

        SettingsSwitchRow {
            title: Strings.get("settings_download_art")
            checked: true
        }

        SettingsSwitchRow {
            title: Strings.get("settings_covers_wifi")
            checked: false
        }

        SettingsSwitchRow {
            title: Strings.get("settings_prefer_folder_art")
            checked: true
        }

        SettingsSwitchRow {
            title: Strings.get("settings_auto_lrc")
            checked: true
        }

        SettingsSelect {
            label: Strings.get("settings_cover_quality")
            options: [
                { value: "high", label: "Alta (1000px, 90%)" },
                { value: "medium", label: "Media (600px, 80%)" },
                { value: "low", label: "Baja (300px, 60%)" }
            ]
            currentValue: "high"
        }
    }
}
